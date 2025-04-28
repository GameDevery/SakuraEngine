export { }

try {
  let a: any
  a.b.c.fuck();
} catch (e) {
  Debug.error(String(e));
}

async function test_async() {
  let a: any
  a.b.c.fuck();
}

await test_async()

let a: any
a.b.c.fuck();

function ImDog(name: string) {
  return (value: any, context: ClassDecoratorContext) => {
    Debug.info(`${name}: ImDog\n  at: <${context.kind}> ${context.name}, meta ${context.metadata}`);
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
Debug.warn(`圆头 来了`)
Debug.error("我们创建了 20w 个 BLAS")

Debug.exit(0)
Debug.info("Hello V8")