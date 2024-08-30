# plot_results.py 
import matplotlib.pyplot as plt
import numpy as np

# Plot Throughput, EED, and PDR vs Number of Nodes
data_nodes = np.loadtxt('results_nodes.txt', skiprows=1)
nodes = data_nodes[:, 0]
throughput_nodes = data_nodes[:, 1]
eed_nodes = data_nodes[:, 2]
pdr_nodes = data_nodes[:, 3]

plt.figure()
plt.plot(nodes, throughput_nodes, 'o-')
plt.xlabel('Number of Nodes')
plt.ylabel('Throughput (Kbps)')
plt.title('Throughput vs Number of Nodes')
plt.ylim(100, 300)  # Set y-axis limits to 100-300 for Throughput
plt.grid(True)
plt.savefig('Throughput_vs_Nodes.png')

plt.figure()
plt.plot(nodes, eed_nodes, 'o-')
plt.xlabel('Number of Nodes')
plt.ylabel('End-to-End Delay (ms)')
plt.title('EED vs Number of Nodes')
plt.ylim(0, 100)  # Set y-axis limits to 0-100 for EED
plt.grid(True)
plt.savefig('EED_vs_Nodes.png')

plt.figure()
plt.plot(nodes, pdr_nodes, 'o-')
plt.xlabel('Number of Nodes')
plt.ylabel('Packet Delivery Ratio (%)')
plt.title('PDR vs Number of Nodes')
plt.ylim(0, 100)  # Set y-axis limits to 50-100 for PDR
plt.grid(True)
plt.savefig('PDR_vs_Nodes.png')

# Plot Throughput, EED, and PDR vs Node Speed
data_speeds = np.loadtxt('results_speeds.txt', skiprows=1)
speeds = data_speeds[:, 0]
throughput_speeds = data_speeds[:, 1]
eed_speeds = data_speeds[:, 2]
pdr_speeds = data_speeds[:, 3]

plt.figure()
plt.plot(speeds, throughput_speeds, 'o-')
plt.xlabel('Node Speed (m/s)')
plt.ylabel('Throughput (Kbps)')
plt.title('Throughput vs Node Speed')
plt.ylim(100, 300)  # Set y-axis limits to 100-300 for Throughput
plt.grid(True)
plt.savefig('Throughput_vs_Speed.png')

plt.figure()
plt.plot(speeds, eed_speeds, 'o-')
plt.xlabel('Node Speed (m/s)')
plt.ylabel('End-to-End Delay (ms)')
plt.title('EED vs Node Speed')
plt.ylim(0, 100)  # Set y-axis limits to 0-100 for EED
plt.grid(True)
plt.savefig('EED_vs_Speed.png')

plt.figure()
plt.plot(speeds, pdr_speeds, 'o-')
plt.xlabel('Node Speed (m/s)')
plt.ylabel('Packet Delivery Ratio (%)')
plt.title('PDR vs Node Speed')
plt.ylim(0, 100)  # Set y-axis limits to 50-100 for PDR
plt.grid(True)
plt.savefig('PDR_vs_Speed.png')

plt.show()
