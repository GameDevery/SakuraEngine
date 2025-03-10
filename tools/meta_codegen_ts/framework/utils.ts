// deno-lint-ignore-file

// code builder
export class CodeBuilder {
  indent_unit: number = 4;
  #indent_stack: number[] = [];
  #indent: number = 0;
  #content: string = "";

  get content(): string { return this.#content; }
  get content_marco(): string { return this.#content.replace(/(\r\n|\n|\r)/g, "\\\n"); }

  // utils
  is_empty(): boolean { return this.#content.length == 0; }

  // indent
  #push_indent(count: number = 1): void {
    this.#indent_stack.push(count);
    this.#indent += count;
  }
  #pop_indent(): void {
    if (this.#indent_stack.length == 0) {
      throw new Error("indent stack is empty");
    }
    this.#indent -= this.#indent_stack.pop()!;
  }

  // codegen tools
  $line(code: string): void {
    if (this.#indent == 0) {
      this.#content += `${code}\n`;
    } else {
      const indent_str = " ".repeat(this.#indent * this.indent_unit);
      this.#content += `${indent_str}${code}\n`;
    }
  }
  $struct(name: string, content: (b: CodeBuilder) => void): void {
    this.$line(`struct ${name} {`);
    this.#push_indent(1);
    content(this);
    this.#pop_indent();
    this.$line(`};`);
  }
  $function(label: string, content: (b: CodeBuilder) => void): void {
    this.$line(`${label} {`)
    this.#push_indent();
    content(this);
    this.#pop_indent();
    this.$line(`}`)
  }
  $switch(value: string, content: (b: CodeBuilder) => void): void {
    this.$line(`switch (${value}) {`)
    this.#push_indent();
    content(this)
    this.#pop_indent();
    this.$line(`}`)
  }
  $if(...pairs: [string, (b: CodeBuilder) => void][]): void {
    pairs.forEach(([cond, block], idx) => {
      // if label
      if (idx === 0) {
        this.$line(`if (${cond}) {`);
      } else {
        if (cond.length > 0) {
          this.$line(`} else if (${cond}) {`);
        } else {
          if (idx !== pairs.length - 1) { throw new Error("else must at last block"); }
          this.$line(`} else {`);
        }
      }

      // block content
      this.#push_indent();
      block(this);
      this.#pop_indent();

      // last
      if (idx === pairs.length - 1) {
        this.$line(`}`);
      }
    })
  }
  $scope(content: (b: CodeBuilder) => void) {
    this.$line(`{`);
    this.#push_indent();
    content(this);
    this.#pop_indent();
    this.$line(`}`);
  }
  $namespace(name: string, content: (b: CodeBuilder) => void): void {
    if (name.length > 0) {
      this.$line(`namespace ${name} {`);
    }
    content(this);
    if (name.length > 0) {
      this.$line(`}`);
    }
  }
  $namespace_line(name: string, content: () => string): void {
    if (name.length > 0) {
      this.$line(`namespace ${name} { ${content()} }`);
    } else {
      this.$line(`${content()}`);
    }
  }
  $indent(content: (b: CodeBuilder) => void): void {
    this.#push_indent();
    content(this);
    this.#pop_indent();
  }


  // to string
  toString(): string {
    return this.#content;
  }
}
