# Experiment 1: Loss Rate, Loss Probability and Latency

On this experiment, it was observed the relation between loss probability, loss rate and latency, and it
was observed that latency goes down as loss probability grows, but loss rate grows as loss probability
also grows.

As seen on Latency vs Loss Probability graph, the latency goes down as the loss probability goes up.
That happens because, since the latency is calculated based on packets arrived, the packets lost 
along the path aren't used on the latency calc, and that explains the relation between latency and
loss probability, because as the loss probability grows, more packets tend to be dropped.
One extra comment is that packets that travels shorter distances tend less to be dropped, because
they traverse fewer hops.

And on the second graph, named Loss Rate vs Loss Probability, the loss rate goes up as the loss probability
goes up, and that relation is simple to be explained. That happens because, as more packets have the chance
to be dropped, more packets are dropped, and so, the loss rate goes up.