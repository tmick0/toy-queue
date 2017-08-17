import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.axes3d import Axes3D

from get_stats import QUEUE_TYPES, QUEUE_SIZES, MESSAGE_SIZES

def main(csv_file):

    series = {k: np.zeros((len(QUEUE_SIZES), len(MESSAGE_SIZES))) for k in QUEUE_TYPES}
    qs_map = {v: k for k, v in enumerate(QUEUE_SIZES)}
    ms_map = {v: k for k, v in enumerate(MESSAGE_SIZES)}

    with open(csv_file, 'r') as fh:
        count = 0
        for line in fh:
        
            count += 1
            if count == 1:
                continue
                
            impl, qsize, msize, time = line.strip().split(", ")
            qsize, msize = int(qsize), int(msize)
            time = float(time)
            
            series[impl][qs_map[qsize], ms_map[msize]] = time
            
    q_grid, m_grid = np.meshgrid(QUEUE_SIZES, MESSAGE_SIZES, indexing='ij')
    max_t = max(map(lambda x: x.max(), series.values()))
    
    fig, axes = plt.subplots(nrows=1, ncols=len(QUEUE_TYPES))
    for (label, data), ax in zip(series.items(), axes):
        im = ax.pcolor(q_grid / 1024, m_grid, data * 1e6 / 1024 / 1024, vmin=0, vmax=max_t)
        ax.set_xlabel("Buffer Size (KiB)")
        ax.set_ylabel("Message Size (B)")
        ax.set_title(label)
        ax.set_xlim(QUEUE_SIZES[0] / 1024, QUEUE_SIZES[-1] / 1024)
        ax.set_ylim(MESSAGE_SIZES[0], MESSAGE_SIZES[-1])
        ax.set_xticks(QUEUE_SIZES / 1024)
        ax.set_yticks(MESSAGE_SIZES)
        
    fig.subplots_adjust(right=0.8)
    cbar = fig.add_axes([0.85, 0.15, 0.05, 0.7])
    cbar.set_title("Throughput (MiB/s)")
    fig.colorbar(im, cax=cbar)
            
    plt.show()
            
    return 0

if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
