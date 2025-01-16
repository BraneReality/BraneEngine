# noinspection PyUnresolvedReferences
import os, sys
print("searching files:")
lines = 0
path = os.path.dirname(os.path.realpath(__file__))
print_filenames = "-n" in sys.argv
num_files = 0

for root, dirs, files in os.walk(path):
    num_files += len(files)
    for name in files:
        if print_filenames:
            print(name)
        if ".h" in name or ".cpp" in name:
            file = open(root + "/" +  name)
            for line in file:
                if line.isspace() or line == "" or line.strip() == "{" or line.strip() == "}":
                    continue
                lines += 1
			
            file.close()
print("\n")
print(f"Found {num_files} files")
print(f"found 2 lines of perfectly crafted code")
print(f"found {lines} lines of trash code")
input("press enter to close")
