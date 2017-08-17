import sys
import itertools
import subprocess
import numpy as np

QUEUE_TYPES   = ["basic_queue", "vm_queue"]
NUM_RUNS      = 10000
QUEUE_SIZES   = np.arange(1, 16) * 4096
MESSAGE_SIZES = np.arange(1, 32) * 256

def main():

    qv, mv = np.meshgrid(QUEUE_SIZES, MESSAGE_SIZES, indexing='ij')
    times = np.zeros(qv.shape)
    
    print("queue_type, queue_size, message_size, time_us")
    for t, q, m in itertools.product(QUEUE_TYPES, QUEUE_SIZES, MESSAGE_SIZES):
        if m <= q-16 :
            p = subprocess.Popen(["./build/benchmark", t, str(NUM_RUNS), str(q), str(m)], stdout=subprocess.PIPE)
            out, err = p.communicate()
            print("%s, %d, %d, %f" % (t, q, m, float(m) / float(out)))

    return 0

if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
