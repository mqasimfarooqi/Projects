from elasticsearch import Elasticsearch
from datetime import datetime
import sys
import os

BATCH_SIZE = 1000
DEST_ES_HOSTS = ['http://127.0.0.1:9200']
DEST_INDEX_NAME = "ag-prod-5520-report-engine-table-data-batches"
SOURCE_ES_HOSTS = ['http://192.168.40.12:9200']
SOURCE_INDEX_NAME = "ag-prod-5520-report-engine-table-data-batches"
TIMESTAMP_FILE = "/tmp/last_processed_submit_time.txt"

def get_last_processed_submit_time(custom_initial_submit_time):
    try:
        with open(TIMESTAMP_FILE, 'r') as f:
            timestamp_str = f.readline().strip()
            if timestamp_str:
                return int(timestamp_str)
    except FileNotFoundError:
        print(f"Timestamp file '{TIMESTAMP_FILE}' not found. Using custom initial submitTime.")
    except ValueError:
        print(f"Error reading timestamp from file. Using custom initial submitTime.")
    
    print(f"Using custom initial submitTime: {custom_initial_submit_time} ({datetime.fromtimestamp(custom_initial_submit_time)})")
    return custom_initial_submit_time

def update_last_processed_submit_time(timestamp):
    with open(TIMESTAMP_FILE, 'w') as f:
        f.write(str(timestamp))

def copy_new_documents(custom_initial_submit_time):
    source_es = Elasticsearch(hosts=SOURCE_ES_HOSTS, request_timeout=30)
    dest_es = Elasticsearch(hosts=DEST_ES_HOSTS, request_timeout=30)

    last_processed_submit_time = get_last_processed_submit_time(custom_initial_submit_time)
    if (last_processed_submit_time != custom_initial_submit_time):
        print(f"Last processed submitTime: {last_processed_submit_time} ({datetime.fromtimestamp(last_processed_submit_time)})")

    new_last_processed_submit_time = last_processed_submit_time

    query = {
        "query": {
            "bool": {
                "must": [
                    {"exists": {"field": "submitTime"}},
                    {"range": {"submitTime": {"gt": last_processed_submit_time}}}
                ]
            }
        },
        "sort": [{"submitTime": {"order": "asc"}}],
        "size": BATCH_SIZE
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
                if "submitTime" in source:
                    new_last_processed_submit_time = max(new_last_processed_submit_time, source["submitTime"])

            if docs_to_index:
                from elasticsearch import helpers
                helpers.bulk(dest_es, docs_to_index)
                print(f"Copied {len(docs_to_index)} new documents. Last processed submitTime updated to: {new_last_processed_submit_time} ({datetime.fromtimestamp(new_last_processed_submit_time) if new_last_processed_submit_time > 0 else 'N/A'})")
                update_last_processed_submit_time(new_last_processed_submit_time)
            else:
                print("No new documents found since the last run. Exiting.")
                sys.exit(0)
        else:
            print("No documents found matching the criteria. Exiting.")
            sys.exit(0)

    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: python {os.path.basename(__file__)} <CUSTOM_INITIAL_SUBMIT_TIME>")
        sys.exit(1)

    try:
        custom_initial_submit_time = int(sys.argv[1])
    except ValueError:
        print("CUSTOM_INITIAL_SUBMIT_TIME must be an integer (Unix epoch time)")
        sys.exit(1)

    while True:
        copy_new_documents(custom_initial_submit_time)
