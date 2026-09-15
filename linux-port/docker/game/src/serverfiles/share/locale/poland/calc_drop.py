import itertools

def probability_at_least_k(drop_table, max_k=None):
    """
    Given a drop_table = { 'item_id': p, ... },
    computes and prints "the probability of getting at least k items"
    for k = 1 up to max_k (or the total number of items if max_k is None).

    Items with p=1.0 are guaranteed (G).
    Other items have p<1.0 and are counted as "optional" (M).

    Probability(≥k) = 1.0 if k <= G;
                      sum_{i=k-G..M} dp[i] otherwise,
    where dp[i] is the prob. exactly i out of M "optional" items drop.
    """
    # Separate guaranteed from non-guaranteed
    guaranteed_probs = [p for p in drop_table.values() if p == 1.0]
    optional_probs   = [p for p in drop_table.values() if p < 1.0]

    G = len(guaranteed_probs)  # number of guaranteed items
    M = len(optional_probs)     # number of optional items
    n = G + M                   # total items

    # Build a DP array for the M optional items
    dp = [0.0] * (M + 1)
    dp[0] = 1.0  # Probability that 0 optional items dropped to start

    for p in optional_probs:
        # update dp in descending order
        for i in range(M, 0, -1):
            dp[i] = dp[i]*(1-p) + dp[i-1]*p
        dp[0] = dp[0]*(1-p)

    # If no max_k given, we do k up to n
    if max_k is None:
        max_k = n

    # For each k in [1..max_k], compute Probability(#drops >= k)
    results = {}
    for k in range(1, max_k + 1):
        if k <= G:
            # We already have G guaranteed drops, so Probability(≥k) = 1
            results[k] = 1.0
        else:
            # Need at least (k - G) from optional
            k_prime = k - G
            if k_prime > M:
                # can't reach k if we have fewer optional items than we need
                results[k] = 0.0
            else:
                # sum dp[i] for i = k_prime..M
                prob_at_least = sum(dp[i] for i in range(k_prime, M + 1))
                results[k] = prob_at_least

    return results

# Example usage
drop_table = {
	"5050": 20,
	"100": 20,
	"1060": 20,
	"2090": 20,
	"7090": 20,
	"5051": 20,
	"11250": 15,
	"11450": 15,
	"11650": 15,
	"11850": 15,
	"101": 15,
	"1061": 15,
	"2091": 15,
	"7091": 15,
	"5052": 15,
	"11251": 15,
	"11451": 15,
	"11651": 15,
	"11851": 15,
	"102": 10,
	"1062": 10,
	"2092": 10,
	"7092": 10,
	"11252": 10,
	"11452": 10,
	"11652": 10,
	"11852": 10,
}

float_drop_table = {}
for key,value in drop_table.items():
	float_drop_table[key] = float(value)/200

# let's say we want the probability for at least k, up to 6 (example)
max_k = 10

# Get the dictionary of { k: Probability(#drops >= k) }
results = probability_at_least_k(float_drop_table, max_k)

# Print them
for k in range(1, max_k + 1):
    print(f"Probability of dropping at least {k} item(s): {results[k]*100:.2f}%")