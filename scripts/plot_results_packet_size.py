import pandas
import os
import glob
import matplotlib.pyplot as plt

packet_sizes = []
latencies = []


def generateLatencyLossProb(latencies, packet_sizes) :
    plt.figure()
    plt.plot(packet_sizes, latencies)

    plt.title("Packets Sizes x Latencies")
    plt.xlabel("Packets Sizes")
    plt.ylabel("Latencies")

    plt.savefig("../results/latency_vs_packet_size.png")


for file in glob.glob("../results/packet_size/*.csv") :
    df = pandas.read_csv(file)
    latencies.append(df["Average Latency"][0])

    parts = file.split("_")
    packet_size = os.path.splitext(parts[4])[0]
    packet_sizes.append(float(packet_size))

sorted_data = sorted(zip(packet_sizes, latencies))
packet_sizes, latencies = zip(*sorted_data)

generateLatencyLossProb(latencies, packet_sizes)