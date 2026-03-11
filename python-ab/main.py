import asyncio
import statistics
import time
from collections import Counter

import aiohttp


async def worker(session, url, method, results, stop_event):
    """Continuous worker loop."""
    while not stop_event.is_set():
        start_time = time.perf_counter()
        try:
            async with session.request(method, url) as response:
                await response.read() 
                duration = time.perf_counter() - start_time
                results.append((response.status, duration))
        except Exception:
            results.append(("Error", 0))

async def run_benchmark(url, method, duration, concurrency, keep_alive=True):
    # Toggle connection pooling vs new connections
    connector = aiohttp.TCPConnector(force_close=not keep_alive)
    
    # ab -k (Keep-Alive) sends this header; otherwise we send 'close'
    headers = {'Connection': 'keep-alive' if keep_alive else 'close'}
    
    results = []
    stop_event = asyncio.Event()

    print(f"Benchmarking {url}")
    print(f"Mode: {'Persistent (Keep-Alive)' if keep_alive else 'New Connection per Request'}")
    print(f"Concurrency: {concurrency} | Duration: {duration}s")
    print("-" * 50)

    async with aiohttp.ClientSession(connector=connector, headers=headers) as session:
        tasks = [
            asyncio.create_task(worker(session, url, method, results, stop_event)) 
            for _ in range(concurrency)
        ]

        await asyncio.sleep(duration)
        stop_event.set()
        await asyncio.gather(*tasks)

    # --- Statistics Engine ---
    total_reqs = len(results)
    if not total_reqs: return print("No data.")

    statuses = Counter([r[0] for r in results])
    latencies = sorted([r[1] * 1000 for r in results if r[0] != "Error"]) # in ms
    
    rps = total_reqs / duration
    avg_lat = statistics.mean(latencies) if latencies else 0

    print(f"Complete requests:      {total_reqs}")
    print(f"Requests per second:    {rps:.2f} [#/sec]")
    print(f"Mean Latency:           {avg_lat:.3f} [ms]")
    
    if latencies:
        print("\nPercentage of the requests served within a certain time (ms)")
        intervals = [50, 66, 75, 80, 90, 95, 98, 99]
        for i in intervals:
            idx = int((i / 100) * len(latencies)) - 1
            print(f"  {i}%    {latencies[idx]:.2f}")
    
    print(f"Status Codes: {dict(statuses)}")

if __name__ == "__main__":
    # CONFIGURATION
    URL = "http://192.168.1.2:3101/auth/sessions"
    METHOD = "PUT"
    TIME = 10
    CONCURRENCY = 100
    
    # Set to True for -k behavior, False for default ab behavior
    KEEP_ALIVE = False 

    asyncio.run(run_benchmark(URL, METHOD, TIME, CONCURRENCY, KEEP_ALIVE))
