export { }
import * as a from "./a"

// @ts-expect-error
Symbol.metadata ??= Symbol('Symbol.metadata');

try {
  let a: any
  a.b.c.fuck();
} catch (e) {
  Debug.error(String(e));
}

function ImDog(name: string) {
  return (value: any, context: ClassDecoratorContext) => {
    Debug.info(`${name}: ImDog\n  at: <${context.kind}> ${context.name}, meta ${typeof context.metadata}`);
    // context.addInitializer(() => {
    //   Debug.info(`${name}: ImDog, ${this.speak}\n  at: <${context.kind}> ${context.name}`);
    // })
    return value;
  }
}


@ImDog("圆头")
class TestClass {
  constructor(
    public speak: string,
  ) {
  }
}

let test_inst = new TestClass("汪汪汪");


Debug.wait(1000)
Debug.info("Hello V8")
Debug.warn(`This is a warning`)
Debug.error("This is an error")

a.duck_speak()

Debug.exit(0)
Debug.info("Hello V8")