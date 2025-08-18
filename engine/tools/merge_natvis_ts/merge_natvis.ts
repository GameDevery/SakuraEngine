import * as xml from 'tsxml'
import { parseArgs } from "util";
import * as fs from 'node:fs'

const { values, positionals } = parseArgs({
  args: Bun.argv,
  options: {
    output: {
      type: 'string',
      short: 'o',
      multiple: false,
    },
  },
  strict: true,
  allowPositionals: true,
})

const output_file = values.output
const input_files = positionals.slice(2)

// output dom
const xml_decl = new xml.ast.ProcessingInstructionNode()
xml_decl.tagName = 'xml'
xml_decl.setAttribute('version', '1.0')
xml_decl.setAttribute('encoding', 'utf-8')

const visualizer_node = new xml.ast.ContainerNode()
visualizer_node.tagName = 'AutoVisualizer'
visualizer_node.setAttribute('xmlns', 'http://schemas.microsoft.com/vstudio/debugger/natvis/2010')

const output_dom = new xml.ast.DocumentNode()
output_dom.appendChild(xml_decl)
output_dom.appendChild(visualizer_node)

// helper
export class LogColor {
  static red(str: string) {
    return `\x1b[31m${str}\x1b[0m`;
  }
  static green(str: string) {
    return `\x1b[32m${str}\x1b[0m`;
  }
  static yellow(str: string) {
    return `\x1b[33m${str}\x1b[0m`;
  }
  static blue(str: string) {
    return `\x1b[34m${str}\x1b[0m`;
  }
  static magenta(str: string) {
    return `\x1b[35m${str}\x1b[0m`;
  }
  static cyan(str: string) {
    return `\x1b[36m${str}\x1b[0m`;
  }
  static white(str: string) {
    return `\x1b[37m${str}\x1b[0m`;
  }
  static gray(str: string) {
    return `\x1b[30m${str}\x1b[0m`;
  }
}

// combine dom
const seen_types = new Set<string>()
for (const file of input_files) {
  console.log(LogColor.green(`-------------processing file: ${file}-------------`))
  const content = fs.readFileSync(file, { encoding: 'utf-8' })
  const dom = await xml.Compiler.parseXmlToAst(content)
  const auto_visualizer_node = dom.childNodes.find(node => node.tagName === 'AutoVisualizer')

  if (auto_visualizer_node instanceof xml.ast.ContainerNode) {
    const type_nodes = auto_visualizer_node.childNodes.filter(node => node.tagName === 'Type')
    console.log(`found ${type_nodes.length} types`)
    for (const type_node of type_nodes) {
      const type_name = type_node.getAttribute('Name')
      const priority = type_node.getAttribute('Priority')
      const compare_key = `${type_name}_${priority}`

      if (seen_types.has(compare_key)) {
        console.log(LogColor.yellow(`[REPEAT] ${type_name} (${priority ?? "DEFAULT"})`))
        continue
      }
      else {
        seen_types.add(compare_key)
        visualizer_node.appendChild(type_node)
        console.log(LogColor.green(`[DONE] ${type_name} (${priority ?? "DEFAULT"})`))
      }

    }
  }
}

// write dom
const output = fs.writeFileSync(output_file as string, output_dom.toString(), { encoding: 'utf-8' })