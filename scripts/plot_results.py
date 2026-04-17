import pandas
import os
import glob
import matplotlib.pyplot as plt

loss_probs = []
loss_rates = []
latencies = []

def generateLossProbLossRate(loss_probs, loss_rates) :
    plt.figure()
    plt.plot(loss_probs, loss_rates)

    plt.title("Loss Probabilites x Loss Rates")
    plt.xlabel("Loss Probabilities")
    plt.ylabel("Loss Rates")

    plt.savefig("../results/loss_rate_vs_loss_prob.png")


def generateLatencyLossProb(latencies, loss_probs) :
    plt.figure()
    plt.plot(loss_probs, latencies)

    plt.title("Loss Probabilities x Latencies")
    plt.xlabel("Loss Probabilities")
    plt.ylabel("Latencies")

    plt.savefig("../results/latency_vs_loss_prob.png")


for file in glob.glob("../results/loss_prob/*.csv") :
    df = pandas.read_csv(file)
    loss_rates.append(df["Loss Rate"][0])
    latencies.append(df["Average Latency"][0])

    parts = file.split("_")
    loss_prob = os.path.splitext(parts[4])[0]
    loss_probs.append(float(loss_prob))

sorted_data = sorted(zip(loss_probs, loss_rates, latencies))
loss_probs, loss_rates, latencies = zip(*sorted_data)

generateLossProbLossRate(loss_probs, loss_rates)
generateLatencyLossProb(latencies, loss_probs)