# Eksperymenty wykonano na grafie "test_structural_4.txt"
# Parametry stałe: cost = 60000, time = 6000

import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import numpy as np


# EKSPERYMENT 1: 
# Rózne wartości max time przy stałym max cost
max_times = np.arange(1000, 13000, 1000)
times = np.array([3078, 4796, 5975, 6384, 6920, 6980, 8167, 7466, 8121, 9642, 
                  10154, 10206])
costs = np.array([173855, 141280, 122371, 116442, 110339, 89606, 94528, 90113, 
                  89914, 88292, 87283, 85302])

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 5))

ax1.plot(max_times, times, color="black", linestyle="dotted", marker='x')
ax1.set_ylabel("Actual Time")
ax1.set_xlabel("Max Time")
ax1.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
ax1.ticklabel_format(axis='y', style='sci', scilimits=(0,0))

ax2.plot(max_times, costs, color="black", linestyle="dotted", marker='x')
ax2.set_ylabel("Actual Cost")
ax2.set_xlabel("Max Time")
ax2.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
ax2.ticklabel_format(axis='y', style='sci', scilimits=(0,0))

# Adjust the layout
plt.suptitle("Max Cost = 60,000")
plt.tight_layout()
plt.show()

# EKSPERYMENT 2: 
# Rózne wartości max cost przy stałym max time
max_costs = np.arange(10000, 130000, 10000)
times = np.array([11022, 11520, 9451, 9831, 7612, 6980, 5918, 7026, 6137, 7127, 
                  5382, 6409])
costs = np.array([79915, 82960, 86203, 85511, 88704, 89606, 92307, 106356,
                  107116, 113804, 118660, 126548])

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 5))

ax1.plot(max_costs, times, color="black", linestyle="dotted", marker='x')
ax1.set_ylabel("Actual Time")
ax1.set_xlabel("Max Cost")
ax1.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
ax1.ticklabel_format(axis='y', style='sci', scilimits=(0,0))

ax2.plot(max_costs, costs, color="black", linestyle="dotted", marker='x')
ax2.set_ylabel("Actual Cost")
ax2.set_xlabel("Max Cost")
ax2.yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
ax2.ticklabel_format(axis='y', style='sci', scilimits=(0,0))

# Adjust the layout
plt.suptitle("Max Time = 6,000")
plt.tight_layout()
plt.show()