from elasticsearch import Elasticsearch, helpers
import logging

# Constants for source and destination Elasticsearch configurations
SRC_ES_HOST = "http://127.0.0.1:9200"  # Replace with the source Elasticsearch host and port
DEST_ES_HOST = "http://192.168.40.14:9200"  # Replace with the destination Elasticsearch host and port

# The index to copy from and to
SRC_INDEX = "ag-prod-5520-network-indicators"  # Replace with the source index name
DEST_INDEX = "ag-prod-5520-network-indicators-at-7"  # Replace with the destination index name

# Connect to the source and destination Elasticsearch instances
src_es = Elasticsearch([SRC_ES_HOST])
dest_es = Elasticsearch([DEST_ES_HOST])

# Set up logging to log failed documents for troubleshooting
logging.basicConfig(filename='failed_docs.log', level=logging.ERROR)

def copy_documents():
    # Scroll query to fetch documents from the source index
    scroll = "2m"  # Scroll time (can be adjusted as needed)
    size = 1000  # Batch size to fetch per scroll
    query = {"query": {"match_all": {}}}  # Fetch all documents from the source index

    # Initial scroll request to get the first batch of documents
    result = src_es.search(index=SRC_INDEX, body=query, scroll=scroll, size=size)

    # The scroll ID to continue fetching data
    scroll_id = result['_scroll_id']
    total_docs = result['hits']['total']['value']
    copied_docs = 0
    print(f"Total documents to copy: {total_docs}")

    # Loop through and copy each batch of documents
    while len(result['hits']['hits']) > 0:
        # Get the number of documents fetched in this batch
        fetched_docs = len(result['hits']['hits'])
        
        # Prepare the documents for bulk indexing to the destination index
        actions = [
            {
                "_op_type": "index",
                "_index": DEST_INDEX,
                "_source": doc["_source"]
            }
            for doc in result['hits']['hits']
        ]

        # Bulk index the documents to the destination index
        try:
            success, failed = helpers.bulk(dest_es, actions)
            copied_docs += fetched_docs  # Track the number of fetched documents (same as pushed here)
            print(f"Fetched {fetched_docs} documents, copied {success} documents in this batch, {failed} failed.")
        except helpers.BulkIndexError as e:
            print(f"Error during bulk indexing: {e}")
            # Log the failed documents for further inspection
            for doc in result['hits']['hits']:
                logging.error(f"Failed to index document: {doc['_id']} - {doc['_source']}")

        # Get the next batch of documents using the scroll ID
        result = src_es.scroll(scroll_id=scroll_id, scroll=scroll)

    # Clear the scroll context
    src_es.clear_scroll(scroll_id=scroll_id)

    print(f"\nTotal documents copied: {copied_docs}")

if __name__ == "__main__":
    copy_documents()
    print("Document copy completed.")
