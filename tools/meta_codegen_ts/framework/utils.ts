// deno-lint-ignore-file

// code builder
export class CodeBuilder {
  indent_unit: number = 4;
  #indent_stack: number[] = [];
  #indent: number = 0;
  #content: string = "";

  get content(): string { return this.#content; }

  // indent
  push_indent(count: number = 1): void {
    this.#indent_stack.push(count);
    this.#indent += count;
  }
  pop_indent(): void {
    if (this.#indent_stack.length == 0) {
      throw new Error("indent stack is empty");
    }
    this.#indent -= this.#indent_stack.pop()!;
  }

  // append content 
  block(code_block: string) : void {
    if (this.#indent == 0) {
      this.#content += code_block;
    } else {
      const lines = code_block.split("\n");
      const indent_str = " ".repeat(this.#indent * this.indent_unit);
      for (const line of lines) {
        this.#content += `${indent_str}${line}\n`;
      }
    }
  }
  line(code: string) : void {
    if (this.#indent == 0) {
      this.#content += code;
    } else {
      const indent_str = " ".repeat(this.#indent * this.indent_unit);
      this.#content += `${indent_str}${code}`;
    }
  }
  wrap(pre: string, callback: (builder: CodeBuilder) => void, post: string): void {
    this.block(pre);
    callback(this);
    this.block(post);
  }
  empty_line(count: number): void {
    this.#content += "\n".repeat(count);
  }
}
