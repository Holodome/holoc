import argparse
import os
import re


def main():
    test_case_str = "TEST_CASE"
    src_directory = "src"
    test_case_names = []
    for filename in os.listdir(src_directory):
        # read only header files to speed things up
        if filename.endswith(".h"):
            with open(filename) as file:
                data = file.read()
                test_case_occurances = [s.start() for s in re.finditer(test_case_str, data)]
                for test_case_idx in test_case_occurances:
                    idx_after_test_case = test_case_idx + len(test_case_str) + 1
                    if data[idx_after_test_case] != "(":
                        print("[ERROR] Incorrecy syntax. ( expected after TEST_CASE")
                        continue
                    test_case_name_idx = idx_after_test_case + 1
                    cursor = test_case_name_idx
                    test_case_name = ""
                    while True:
                        if cursor >= len(data):
                            break
                        symb = data[cursor]
                        if symb.isalpha():
                            test_case_name += symb
                        else:
                            break
                        cursor += 1
                    
                    if test_case_name in test_case_names:
                        print(f"[ERROR] Test case '{test_case_name} already exists'")
                    else:
                        test_case_names.append(test_case_name)

if __name__ == "__main__":
    main()