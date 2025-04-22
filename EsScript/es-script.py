from elasticsearch import Elasticsearch, helpers
import logging
from copy import deepcopy

# Constants for source and destination Elasticsearch configurations
SRC_ES_HOST = "http://192.168.40.14:9200"
DEST_ES_HOST = "http://192.168.40.14:9200"

# The index to copy from and to
SRC_INDEX = "change"
DEST_INDEX = "change"

# Batch size for bulk operations
BATCH_SIZE = 1000

# Set up Elasticsearch clients
src_es = Elasticsearch([SRC_ES_HOST])
dest_es = Elasticsearch([DEST_ES_HOST])

# Set up logging
logging.basicConfig(
    filename='failed_docs.log',
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# _agCoreInfo block to be added or overwritten in each document
AG_CORE_INFO = {
    "_agCoreInfo": {
        "vantage": {
            "application": {
                "id": "2542",
                "name": "gee-holmdeldc-ag1-ag-core"
            },
            "customer": {
                "id": "1",
                "name": "Anuvu"
            },
            "environment": "prod",
            "facility": {
                "id": "3714",
                "name": "Aviation Holmdel Datacenter"
            }
        },
        "version": None  # Do not overwrite version if it's already set
    }
}

def deep_merge_ag_core_info(existing, update):
    for key, value in update.items():
        if isinstance(value, dict):
            existing[key] = deep_merge_ag_core_info(existing.get(key, {}), value)
        elif value not in [None, "", 0]:
            existing[key] = value
    return existing

def copy_documents():
    scroll = "2m"
    size = BATCH_SIZE
    query = {"query": {"match_all": {}}}

    # Initial search to get the scroll ID
    result = src_es.search(index=SRC_INDEX, body=query, scroll=scroll, size=size)
    scroll_id = result['_scroll_id']
    total_docs = result['hits']['total']['value']

    print(f"Total documents to copy: {total_docs}")

    total_success = 0
    total_failed = 0
    already_had_agcoreinfo_count = 0
    batch_num = 0

    while True:
        hits = result['hits']['hits']
        if not hits:
            break

        batch_num += 1
        actions = []
        for doc in hits:
            doc_id = doc["_id"]
            source = doc["_source"]

            if "_agCoreInfo" in source:
                already_had_agcoreinfo_count += 1
                logging.info(f"_agCoreInfo exists in doc ID: {doc_id}, merging fields.")
                existing_ag_core_info = source["_agCoreInfo"]
            else:
                existing_ag_core_info = {}

            merged_ag_core_info = deep_merge_ag_core_info(deepcopy(existing_ag_core_info), AG_CORE_INFO["_agCoreInfo"])
            source["_agCoreInfo"] = merged_ag_core_info

            actions.append({
                "_op_type": "index",
                "_index": DEST_INDEX,
                "_id": doc_id,  # Retain original ID
                "_source": source
            })

        try:
            response = helpers.bulk(dest_es, actions, raise_on_error=False, raise_on_exception=False)
            success_count = response[0]
            failure_count = len(response[1])
            total_success += success_count
            total_failed += failure_count

            print(f"Batch {batch_num}: {len(hits)} docs processed | {success_count} success | {failure_count} failed")

            for fail in response[1]:
                doc_id = fail.get('index', {}).get('_id', 'UNKNOWN')
                reason = fail.get('index', {}).get('error', {}).get('reason', 'No reason provided')
                logging.error(f"Failed to index doc ID {doc_id}: {reason}")

        except Exception as e:
            logging.exception("Exception occurred during bulk indexing")

        # Get next scroll page
        result = src_es.scroll(scroll_id=scroll_id, scroll=scroll)

    # Clear the scroll context
    src_es.clear_scroll(scroll_id=scroll_id)

    print("\nCopy completed.")
    print(f"Total documents copied successfully: {total_success}")
    print(f"Total documents failed: {total_failed}")
    print(f"Documents that already had _agCoreInfo and were merged: {already_had_agcoreinfo_count}")


if __name__ == "__main__":
    copy_documents()
