import asyncio
import time
from collections import Counter

import aiohttp


async def worker(session, url, method, results, stop_event):
    """A persistent worker that keeps sending requests until the time is up."""
    while not stop_event.is_set():
        start_time = time.perf_counter()
        try:
            # Using the dynamic method (PUT, POST, GET, etc.)
            async with session.request(method, url) as response:
                await response.read() 
                duration = time.perf_counter() - start_time
                results.append((response.status, duration))
        except Exception:
            results.append(("Error", 0))

async def run_timed_benchmark(url, method, duration_secs, concurrency):
    results = []
    stop_event = asyncio.Event()

    print(f"Benchmarking {url}...")
    print(f"Method: {method} | Concurrency: {concurrency} | Duration: {duration_secs}s")
    print("--------------------------------------------------")

    async with aiohttp.ClientSession() as session:
        # Create 'concurrency' number of workers
        tasks = [
            asyncio.create_task(worker(session, url, method, results, stop_event)) 
            for _ in range(concurrency)
        ]

        # Wait for the specified duration
        await asyncio.sleep(duration_secs)
        
        # Signal workers to stop and wait for them to wrap up current request
        stop_event.set()
        await asyncio.gather(*tasks)

    # Statistics Calculation
    total_reqs = len(results)
    statuses = Counter([r[0] for r in results])
    latencies = [r[1] for r in results if r[0] != "Error"]
    
    # Summary Output
    rps = total_reqs / duration_secs
    avg_lat = (sum(latencies) / len(latencies) * 1000) if latencies else 0

    print(f"Complete requests:      {total_reqs}")
    print(f"Requests per second:    {rps:.2f} [#/sec] (mean)")
    print(f"Time per request:       {avg_lat:.3f} [ms] (mean)")
    print("Status Codes:")
    for code, count in statuses.items():
        print(f"  {code}: {count}")

if __name__ == "__main__":
    # Settings from your command: ab -m PUT -t 10 -c 100
    TARGET_URL = "http://192.168.1.2:3101/auth/sessions"
    METHOD = "PUT"
    SECONDS = 10
    CONCURRENCY = 100

    asyncio.run(run_timed_benchmark(TARGET_URL, METHOD, SECONDS, CONCURRENCY))
