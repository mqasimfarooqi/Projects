from elasticsearch import Elasticsearch, helpers
from datetime import datetime
import sys
import os

BATCH_SIZE = 1000
DEST_ES_HOSTS = ['http://127.0.0.1:9200']
SOURCE_ES_HOSTS = ['http://192.168.40.12:9200']
TIMESTAMP_FILE = "/tmp/last_processed_time.txt"

# Define index configs with time field per index
INDICES_TO_COPY = [
    {"source": "ag-prod-5523-events", "dest": "ag-prod-5523-events", "time_field": "id.timestamp"},
    {"source": "ag-prod-5520-network-indicators", "dest": "ag-prod-5520-network-indicators", "time_field": "endTime"},
    {"source": "ag-prod-5520-report-engine-device-items", "dest": "ag-prod-5520-report-engine-device-items", "time_field": "timestamp"},
]

def get_last_processed_time(default_time_str):
    try:
        with open(TIMESTAMP_FILE, 'r') as f:
            ts_str = f.readline().strip()
            if ts_str:
                return ts_str
    except FileNotFoundError:
        print(f"Timestamp file '{TIMESTAMP_FILE}' not found. Using provided initial timestamp.")
    return default_time_str

def update_last_processed_time(new_time_str):
    with open(TIMESTAMP_FILE, 'w') as f:
        f.write(new_time_str)

def copy_documents(default_start_time):
    source_es = Elasticsearch(hosts=SOURCE_ES_HOSTS, request_timeout=30)
    dest_es = Elasticsearch(hosts=DEST_ES_HOSTS, request_timeout=30)

    last_processed_time = get_last_processed_time(default_start_time)
    print(f"Copying documents with timestamp > {last_processed_time}")

    for index_info in INDICES_TO_COPY:
        source_index = index_info["source"]
        dest_index = index_info["dest"]
        time_field = index_info["time_field"]

        print(f"\nProcessing index: {source_index}")

        query = {
            "query": {
                "range": {
                    time_field: {
                        "gt": last_processed_time
                    }
                }
            },
            "sort": [{time_field: {"order": "asc"}}],
            "size": BATCH_SIZE
        }

        try:
            res = source_es.search(index=source_index, body=query)
            docs_to_index = []
            max_time = last_processed_time

            for hit in res['hits']['hits']:
                source_doc = hit['_source']

                # Placeholder for any modifications you want to make
                modified_doc = source_doc

                # Update max timestamp
                doc_time = get_nested_value(modified_doc, time_field)
                if doc_time and doc_time > max_time:
                    max_time = doc_time

                docs_to_index.append({
                    "_index": dest_index,
                    "_id": hit["_id"],
                    "_source": modified_doc
                })

            if docs_to_index:
                helpers.bulk(dest_es, docs_to_index)
                print(f"Copied {len(docs_to_index)} docs to {dest_index}, last timestamp: {max_time}")
                update_last_processed_time(max_time)
            else:
                print("No new documents found.")

        except Exception as e:
            print(f"Error processing index {source_index}: {e}")

def get_nested_value(data, dotted_key):
    keys = dotted_key.split('.')
    for key in keys:
        if isinstance(data, dict) and key in data:
            data = data[key]
        else:
            return None
    return data

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: python {os.path.basename(__file__)} <INITIAL_TIMESTAMP_ISO8601>")
        print(f"Example: python {os.path.basename(__file__)} 2025-04-01T00:00:00Z")
        sys.exit(1)

    initial_timestamp = sys.argv[1]
    copy_documents(initial_timestamp)
