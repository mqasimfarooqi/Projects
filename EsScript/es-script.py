from elasticsearch import Elasticsearch, helpers
from datetime import datetime
import sys
import os
import time

BATCH_SIZE = 1000
REQUEST_TIMEOUT = 120
DEST_ES_HOSTS = ['http://127.0.0.1:9200']
SOURCE_ES_HOSTS = ['http://192.168.40.7:9200']
TIMESTAMP_FILE = "/tmp/last_processed_time.txt"

INDICES_TO_COPY = [
    # {"source": "ag-prod-5520-events", "dest": "ag-prod-5523-events", "time_field": "id.timestamp"},
    {"source": "ag-prod-5520-network-indicators", "dest": "ag-prod-5520-network-indicators", "time_field": "endTime"},
    # {"source": "ag-prod-5520-report-engine-device-items", "dest": "ag-prod-5520-report-engine-device-items", "time_field": "timestamp"},
]

def get_last_processed_time(default_time_str):
    """Reads the last processed timestamp from the file, or returns the default."""
    try:
        with open(TIMESTAMP_FILE, 'r') as f:
            ts_str = f.readline().strip()
            if ts_str:
                print(f"Found timestamp file '{TIMESTAMP_FILE}'. Starting from: {ts_str}")
                return ts_str
            else:
                print(f"Timestamp file '{TIMESTAMP_FILE}' is empty. Using provided initial timestamp: {default_time_str}")
                return default_time_str
    except FileNotFoundError:
        print(f"Timestamp file '{TIMESTAMP_FILE}' not found. Using provided initial timestamp: {default_time_str}")
    except Exception as e:
        print(f"Error reading timestamp file '{TIMESTAMP_FILE}': {e}. Using provided initial timestamp: {default_time_str}")
    return default_time_str

def update_last_processed_time(new_time_str):
    """Writes the new latest processed timestamp to the file."""
    try:
        with open(TIMESTAMP_FILE, 'w') as f:
            f.write(new_time_str)
        print(f"Updated timestamp file '{TIMESTAMP_FILE}' to: {new_time_str}")
    except Exception as e:
        print(f"Error writing timestamp file '{TIMESTAMP_FILE}': {e}")

def get_nested_value(data, dotted_key):
    """Safely retrieves a value from a nested dictionary using a dot-separated key."""
    keys = dotted_key.split('.')
    value = data
    try:
        for key in keys:
            value = value[key]
        return value
    except (TypeError, KeyError, IndexError):
        print(f"Warning: Could not retrieve nested key '{dotted_key}' from document.")
        return None

def copy_documents(default_start_time):
    """Copies documents from source to destination ES, continuing until caught up."""
    try:
        source_es = Elasticsearch(hosts=SOURCE_ES_HOSTS, request_timeout=REQUEST_TIMEOUT)
        dest_es = Elasticsearch(hosts=DEST_ES_HOSTS, request_timeout=REQUEST_TIMEOUT)
        source_es.ping()
        dest_es.ping()
        print("Successfully connected to Source and Destination Elasticsearch.")
    except Exception as e:
        print(f"Error connecting to Elasticsearch: {e}")
        sys.exit(1)

    # Get the timestamp from which to start this run
    last_processed_time_from_file = get_last_processed_time(default_start_time)
    # This will track the highest timestamp processed across all indices in this run
    overall_max_time_this_run = last_processed_time_from_file
    total_docs_copied_this_run = 0

    print(f"Starting copy process for documents with timestamp > {last_processed_time_from_file}")

    for index_info in INDICES_TO_COPY:
        source_index = index_info["source"]
        dest_index = index_info["dest"]
        time_field = index_info["time_field"]
        docs_copied_for_index = 0
        index_fully_caught_up = False

        # Use the timestamp from the file as the starting point for this index's first query
        current_start_time_for_index = last_processed_time_from_file
        # Track the max time found specifically for this index during this run
        max_time_for_index = last_processed_time_from_file

        print(f"\n--- Processing index: {source_index} -> {dest_index} ---")
        print(f"Starting search for {source_index} with {time_field} > {current_start_time_for_index}")

        # Inner loop: Keep fetching batches for the current index until no more new docs are found
        while True:
            query = {
                "query": {
                    "range": {
                        time_field: {
                            "gt": current_start_time_for_index # Use the timestamp of the last doc seen *in the previous batch for this index*
                        }
                    }
                },
                "sort": [{time_field: {"order": "asc"}}],
                "size": BATCH_SIZE
            }

            try:
                print(f"Searching {source_index} for batch with {time_field} > {current_start_time_for_index}...")
                res = source_es.search(index=source_index, body=query)
                hits = res['hits']['hits']
                num_hits = len(hits)

                if num_hits == 0:
                    print(f"No more documents found for {source_index} with timestamp > {current_start_time_for_index}.")
                    index_fully_caught_up = True
                    break # Exit the inner while loop for this index

                docs_to_index = []
                batch_max_time = current_start_time_for_index # Initialize with the lower bound for this batch

                for hit in hits:
                    source_doc = hit['_source']
                    doc_time = get_nested_value(source_doc, time_field)

                    if doc_time is None:
                         print(f"Warning: Skipping document ID {hit['_id']} due to missing time field '{time_field}'.")
                         continue # Skip docs where time field is missing

                    # Update the maximum time seen in this specific batch
                    if doc_time > batch_max_time:
                        batch_max_time = doc_time

                    # Prepare document for bulk indexing
                    docs_to_index.append({
                        "_index": dest_index,
                        "_id": hit["_id"],
                        "_source": source_doc # Use original source doc, modify if needed
                    })

                if docs_to_index:
                    try:
                        print(f"Indexing {len(docs_to_index)} documents to {dest_index}...")
                        helpers.bulk(dest_es, docs_to_index, request_timeout=REQUEST_TIMEOUT)
                        docs_copied_for_index += len(docs_to_index)
                        total_docs_copied_this_run += len(docs_to_index)

                        # Update the max time seen for this index
                        max_time_for_index = batch_max_time
                        # Update the overall max time seen across all indices in this run
                        if max_time_for_index > overall_max_time_this_run:
                             overall_max_time_this_run = max_time_for_index

                        print(f"Successfully indexed {len(docs_to_index)} docs. Last timestamp in batch: {batch_max_time}. Total for {source_index}: {docs_copied_for_index}")

                        # Set the start time for the *next* query for *this index* to the latest time seen in *this* batch
                        current_start_time_for_index = batch_max_time

                    except helpers.BulkIndexError as bie:
                        print(f"Bulk indexing error occurred for {dest_index}: {len(bie.errors)} errors.")
                        # Log first few errors for debugging
                        for i, error in enumerate(bie.errors[:5]):
                             print(f"  Error {i+1}: {error}")
                        # Decide if you want to retry, skip, or halt. Here we'll continue to the next batch.
                        print("Continuing to next batch despite errors...")
                        current_start_time_for_index = batch_max_time # Ensure progress past the problematic batch
                    except Exception as e_bulk:
                        print(f"Error during bulk indexing to {dest_index}: {e_bulk}")
                        # Potentially break or implement retry logic
                        break # Exit inner loop for this index on critical bulk error
                else:
                    # This case should technically be covered by num_hits == 0, but good as a safeguard
                    print(f"No valid documents found in the batch for {source_index} despite {num_hits} hits (check time field extraction).")
                    break # Exit inner while loop


            except Exception as e_search:
                print(f"Error searching index {source_index}: {e_search}")
                # Decide how to handle search errors, e.g., retry, skip index, exit
                break # Exit inner while loop for this index on search error

        # End of inner while loop for the current index
        if index_fully_caught_up:
             print(f"--- Finished processing index: {source_index}. Copied {docs_copied_for_index} documents. Caught up. ---")
        else:
             print(f"--- Finished processing index: {source_index} due to error or interruption. Copied {docs_copied_for_index} documents. ---")

    # After processing all indices
    print(f"\n--- Full copy cycle completed ---")
    print(f"Total documents copied across all indices in this run: {total_docs_copied_this_run}")

    # Update the timestamp file *only if* we processed any documents beyond the initial start time
    if overall_max_time_this_run > last_processed_time_from_file:
        print(f"Highest timestamp processed in this run: {overall_max_time_this_run}")
        update_last_processed_time(overall_max_time_this_run)
    else:
        print(f"No new documents were found across any index beyond {last_processed_time_from_file}. Timestamp file not updated.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: python {os.path.basename(__file__)} <INITIAL_TIMESTAMP_ISO8601>")
        print(f"Example: python {os.path.basename(__file__)} 2025-04-01T00:00:00Z")
        print("\nNote: The script will use the timestamp from '{TIMESTAMP_FILE}' if it exists and is not empty,")
        print("otherwise it will use the <INITIAL_TIMESTAMP_ISO8601> provided.")
        sys.exit(1)

    initial_timestamp_arg = sys.argv[1]
    # Basic validation of the timestamp format (optional but recommended)
    try:
        datetime.fromisoformat(initial_timestamp_arg.replace('Z', '+00:00'))
    except ValueError:
        print(f"Error: Invalid ISO8601 timestamp format provided: '{initial_timestamp_arg}'")
        print("Please use the format YYYY-MM-DDTHH:MM:SSZ (e.g., 2025-04-01T00:00:00Z)")
        sys.exit(1)

    copy_documents(initial_timestamp_arg)
    print("\nScript finished.")
