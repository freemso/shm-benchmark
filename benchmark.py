import subprocess
import os
import errno

test_data_file_list = ["16M.data", "64M.data", "1G.data"]
test_data_size_list = [16, 64, 1024] # in MB
buf_size_in_kb_list = [16, 64, 256] # in KB


def benchmark(prog):
    result = ""
    for buf_size_in_kb in buf_size_in_kb_list:
        buf_size = buf_size_in_kb * 1024
        for test_data_file, test_data_size in zip(test_data_file_list, test_data_size_list):
            cp = subprocess.run([prog, str(buf_size), test_data_file, test_data_file+".copy"], stdout=subprocess.PIPE)
            p_output = cp.stdout.decode("utf8")
            result += "Buffer size: {} kilobytes.\tData size: {} megabytes.\tTime cost: {} seconds.\n\n".format(str(buf_size_in_kb), str(test_data_size), str(p_output))
    return result

shm_result = benchmark("./shm.out")
cpy_result = benchmark("./cpy.out")

output_dir = "./output/"
if not os.path.exists(output_dir):
    try:
        os.makedirs(output_dir)
    except OSError as exc: # Guard against race condition
        if exc.errno != errno.EEXIST:
            raise

# Write to output file
with open(output_dir + "shm.txt", "w") as f:
    f.write(shm_result)
    f.close()

with open(output_dir + "cpy.txt", "w") as f:
    f.write(cpy_result)
    f.close()