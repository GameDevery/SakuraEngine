import * as ml from '@framework/meta_lang'
import * as t from 'bun:test'

class TestMetaLang {
  // value assign
  @ml.value('number')
  number_val: number = 0;
  @ml.value('boolean')
  boolean_val: boolean = false;
  @ml.value('string')
  string_val: string = "";

  // array assign
  @ml.array('number')
  number_arr_val: number[] = [];
  @ml.array('boolean')
  boolean_arr_val: boolean[] = [];
  @ml.array('string')
  string_arr_val: string[] = [];

  // preset
  @ml.preset('default')
  preset_default() {
    this.number_val = 0;
    this.boolean_val = false;
    this.string_val = "";

    this.number_arr_val = [];
    this.boolean_arr_val = [];
    this.string_arr_val = [];
  }

  // sub object
  sub: TestMetaLangSub = new TestMetaLangSub();
}
class TestMetaLangSub {
  @ml.value('number')
  value: number = 0;

  @ml.array('number')
  array_value: number[] = [];

  @ml.value_proxy('number')
  proxy_value(v: number) {
    this.value = v;
  }

  @ml.array_proxy('number')
  proxy_array(v: number[], is_append: boolean) {
    if (is_append) {
      this.array_value.push(...v);
    } else {
      this.array_value = v;
    }
  }
};

const compiler = ml.Compiler.load();
const program = compiler.compile(
  `
  test.number_val = 114;
  test.boolean_val = true;
  test.string_val = "114514";

  test = @default;

  test.number_arr_val = [1, 2, 3];
  test.boolean_arr_val = [true, false, true];
  test.string_arr_val = ["1", "2", "3"];

  test.sub = 114514;
  test.sub = [1, 1, 4, 5, 1, 4];
  `
);
const test_obj = {
  test: new TestMetaLang()
};
program.exec(test_obj);
console.log(test_obj);