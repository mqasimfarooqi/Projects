from elasticsearch import Elasticsearch
import time
import json
from datetime import datetime

# Configuration for the source and destination Elasticsearch clusters
SOURCE_ES_HOSTS = ['http://192.168.40.12:9200']  # Replace with the actual IP and port
DEST_ES_HOSTS = ['http://127.0.0.1:9200']  # Replace with the actual IP and port
SOURCE_INDEX_NAME = "ag-prod-5520-report-engine-table-data-batches"
DEST_INDEX_NAME = "ag-prod-5520-report-engine-table-data-batches-backup"  # You might want a different name
TIMESTAMP_FILE = "/tmp/last_processed_time.txt"  # Adjust the path as needed

# Set a custom initial timestamp here (in Unix epoch seconds)
CUSTOM_INITIAL_TIMESTAMP = 1744303316  # Replace with your desired starting timestamp

def get_last_processed_time():
    try:
        with open(TIMESTAMP_FILE, 'r') as f:
            timestamp_str = f.readline().strip()
            if timestamp_str:
                return int(timestamp_str)
    except FileNotFoundError:
        print(f"Timestamp file '{TIMESTAMP_FILE}' not found. Using custom initial timestamp.")
        return CUSTOM_INITIAL_TIMESTAMP
    except ValueError:
        print(f"Error reading timestamp from file. Using custom initial timestamp.")
        return CUSTOM_INITIAL_TIMESTAMP
    return CUSTOM_INITIAL_TIMESTAMP # Return custom if file is empty

def update_last_processed_time(timestamp):
    with open(TIMESTAMP_FILE, 'w') as f:
        f.write(str(timestamp))

def copy_new_documents():
    source_es = Elasticsearch(hosts=SOURCE_ES_HOSTS, request_timeout=30)
    dest_es = Elasticsearch(hosts=DEST_ES_HOSTS, request_timeout=30)

    last_processed = get_last_processed_time()
    new_last_processed = last_processed

    query = {
        "query": {
            "bool": {
                "must": [
                    {"exists": {"field": "submitTime"}},
                    {"range": {"createTime": {"gt": last_processed}}}
                ]
            }
        },
        "sort": [{"createTime": {"order": "asc"}}],
        "size": 1000
    }

    try:
        res = source_es.search(index=SOURCE_INDEX_NAME, body=query)
        docs_to_index = []

        if res['hits']['hits']:
            for hit in res['hits']['hits']:
                source = hit['_source']
                action = {
                    "_index": DEST_INDEX_NAME,
                    "_id": hit['_id'],
                    "_source": source
                }
                docs_to_index.append(action)
                new_last_processed = max(new_last_processed, source.get("createTime", last_processed))

            if docs_to_index:
                from elasticsearch import helpers
                helpers.bulk(dest_es, docs_to_index)
                print(f"Copied {len(docs_to_index)} new documents. Last processed time updated to: {new_last_processed} ({datetime.fromtimestamp(new_last_processed)})")
            else:
                print("No new documents found since the last run.")
        else:
            print("No documents found matching the criteria.")

        update_last_processed_time(new_last_processed)

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    while True:
        copy_new_documents()
        time.sleep(5)
