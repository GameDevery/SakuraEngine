# FROM: https://github.com/microsoft/vscode-cpptools/issues/10917#issuecomment-2189106550
import os
import sys
import xml.etree.ElementTree as ET
import argparse

natvis_namespace = "http://schemas.microsoft.com/vstudio/debugger/natvis/2010"

# colored log
class LogHelper:
    def red(raw_msg: str) -> str:
        return f"\033[31m{raw_msg}\033[0m"

    def green(raw_msg: str) -> str:
        return f"\033[32m{raw_msg}\033[0m"

    def yellow(raw_msg: str) -> str:
        return f"\033[33m{raw_msg}\033[0m"

    def blue(raw_msg: str) -> str:
        return f"\033[34m{raw_msg}\033[0m"

    def cyan(raw_msg: str) -> str:
        return f"\033[36m{raw_msg}\033[0m"

    def magenta(raw_msg: str) -> str:
        return f"\033[35m{raw_msg}\033[0m"

    def gray(raw_msg: str) -> str:
        return f"\033[30m{raw_msg}\033[0m"

# register natvis namespace for natvis
ET.register_namespace('', natvis_namespace)

# merge natvis files
def concatenate_natvis_files(natvis_files, output_path):
    
    # output element
    auto_visualizer = ET.Element('AutoVisualizer')
    
    # process namespace
    namespaces_alias = {"vis": natvis_namespace} # namespace alias for iterfind with XPath

    # process natvis files
    seen_types = set() # prevent duplicate types
    for natvis_file in natvis_files:
        # parse natvis files
        print(LogHelper.green(f"-------------processing file: {natvis_file}-------------"))
        tree = ET.parse(natvis_file)
        root = tree.getroot()
        print(f"found {len(root)} types")

        # each type element and merge to output
        for type_element in root.iterfind('vis:Type', namespaces=namespaces_alias):
            # check if type is already in seen_types
            type_name = type_element.get('Name')
            if type_name in seen_types:
                print(LogHelper.yellow(f"[REPEAT] {type_name}"))
                continue
            else:
                # add to output document
                seen_types.add(type_name)
                auto_visualizer.append(type_element)
                print(LogHelper.green(f"[DONE] {type_name}"))

    # build document
    tree = ET.ElementTree(auto_visualizer)
    ET.indent(tree, space="\t", level=0)
    
    # write document to file
    tree.write(output_path, encoding='utf-8', xml_declaration=True, method='xml')

if __name__ == "__main__":
    # parse args
    parser = argparse.ArgumentParser(description="merge natvis to a single file")
    parser.add_argument('-o', '--output', help="output file.", required=True, type=str)
    parser.add_argument('natvis_files', nargs='+', help="natvis files to merge", type=str)
    args = parser.parse_args()
    
    # do merge
    concatenate_natvis_files(args.natvis_files, args.output)