from elasticsearch import Elasticsearch, helpers
import logging

# Constants for source and destination Elasticsearch configurations
SRC_ES_HOST = "http://192.168.40.14:9200"  # Replace with the source Elasticsearch host and port
DEST_ES_HOST = "http://127.0.0.1:9200"     # Replace with the destination Elasticsearch host and port

# The index to copy from and to
SRC_INDEX = "ag-prod-5520-network-indicators"   # Replace with the source index name
DEST_INDEX = "ag-prod-5520-network-indicators"  # Replace with the destination index name

# Connect to the source and destination Elasticsearch instances
src_es = Elasticsearch([SRC_ES_HOST])
dest_es = Elasticsearch([DEST_ES_HOST])

# Set up logging to log failed documents for troubleshooting
logging.basicConfig(filename='failed_docs.log', level=logging.ERROR)

# _agCoreInfo block to be added to each document
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
        "version": None
    }
}

def copy_documents():
    scroll = "2m"  # Scroll time (can be adjusted as needed)
    size = 1000    # Batch size to fetch per scroll
    query = {"query": {"match_all": {}}}

    result = src_es.search(index=SRC_INDEX, body=query, scroll=scroll, size=size)
    scroll_id = result['_scroll_id']
    total_docs = result['hits']['total']['value']
    copied_docs = 0

    print(f"Total documents to copy: {total_docs}")

    while len(result['hits']['hits']) > 0:
        fetched_docs = len(result['hits']['hits'])

        actions = []
        for doc in result['hits']['hits']:
            source = doc["_source"]
            source.update(AG_CORE_INFO)  # Add the _agCoreInfo block to each document
            actions.append({
                "_op_type": "index",
                "_index": DEST_INDEX,
                "_source": source
            })

        try:
            success, failed = helpers.bulk(dest_es, actions)
            copied_docs += success
            print(f"Fetched {fetched_docs} docs, copied {success} successfully, {failed} failed.")
        except helpers.BulkIndexError as e:
            print(f"Bulk indexing error: {e}")
            for doc in result['hits']['hits']:
                logging.error(f"Failed to index document: {doc['_id']} - {doc['_source']}")

        result = src_es.scroll(scroll_id=scroll_id, scroll=scroll)

    src_es.clear_scroll(scroll_id=scroll_id)
    print(f"\nTotal documents copied: {copied_docs}")

if __name__ == "__main__":
    copy_documents()
    print("Document copy completed.")
