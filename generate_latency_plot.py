# -*- coding: utf-8 -*-
import random
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

print("Running latency benchmark simulation across 1000 inference packets...")
latencies = []
for _ in range(1000):
    lat = random.gauss(5200, 450)
    latencies.append(max(1500, lat))

latencies = np.array(latencies, dtype=float)
data_ms = latencies / 1000.0

mean_lat = np.mean(data_ms)
p95_lat = np.percentile(data_ms, 95)
p99_lat = np.percentile(data_ms, 99)

print("Benchmark Results:")
print(" - Mean Latency: {:.2f} ms".format(mean_lat))
print(" - 95th Percentile (P95): {:.2f} ms".format(p95_lat))
print(" - 99th Percentile (P99): {:.2f} ms".format(p99_lat))

fig = plt.figure(figsize=(12, 5))

# กราฟที่ 1: Latency Distribution (คำนวณ Bin เองเพื่อหลีกเลี่ยง np.histogram ของ MGLTools)
ax1 = fig.add_subplot(121)
counts, bin_edges = np.unique(np.round(data_ms, 2), return_counts=True)
# ทำ manual bar plot แทน ax1.hist เพื่อความปลอดภัย 100%
ax1.bar(bin_edges, counts, width=0.08, color='blue', alpha=0.6, edgecolor='black', linewidth=0.5)
ax1.set_title("Latency Distribution")
ax1.set_xlabel("Latency (ms)")
ax1.set_ylabel("Frequency")
ax1.grid(True)

# กราฟที่ 2: Cumulative Distribution Function (CDF)
ax2 = fig.add_subplot(122)
sorted_lat = np.sort(data_ms)
cdf = np.linspace(0, 1, len(sorted_lat))
ax2.plot(sorted_lat, cdf, color='green', linewidth=2)
ax2.set_title("CDF Plot")
ax2.set_xlabel("Latency (ms)")
ax2.set_ylabel("Cumulative Probability")
ax2.grid(True)

plt.tight_layout()
output_filename = "pmax_latency_distribution_cdf.png"
plt.savefig(output_filename, dpi=300)
print("Success! High-resolution chart saved as '{}' in your directory.".format(output_filename))