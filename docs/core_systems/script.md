# SakuraEngine 脚本系统

## 概述

脚本系统架设再 RTTR 系统之上，旨在为**渐进类型脚本**（例如 lua/javascript/python）提供统一的绑定范式。

由于架构在 RTTR 系统之上，即便没有反射系统的介入，也可以通过手动导出的形式为类型支持脚本功能。

反射系统介入的方式是使用一系列预定义的 meta 标记：
- `sscript_visible`：标记类型/字段/函数为脚本可见
- `sscript_newable`：标记类型为脚本可创建类型
- `sscript_mapping`：标记类型为 mapping 导出
- `sscript_mixin`：标记函数为脚本可混入（功能等效于重载）
- `sscript_getter`：标记函数为脚本模拟 property 的 getter
- `sscript_setter`：标记函数为脚本模拟 property 的 setter
- `sparam_in/sparam_out/sparam_inout`：标记参数的行为

## 脚本导出概念
### Primitive（基元类型）
主要包括 number、boolean、string 等基础类型，以 javascript 为例，类型映射如下：
- `int8/int16/int32` => `number`
- `uint8/uint16/uint32` => `number`
- `float/double` => `number`
- `int64/uint64` => `BigInt`
- `bool` => `boolean`
- `String/StringView` => `string`

### Mapping（映射导出）
通常来说，导出对象/结构体需要一个脚本对象携带一个 c++ 对象，访问操作直接操作 c++ 内存，这就造成了两次内存分配，以及一系列复杂的内存访问，在某些情况下其开销是较大的（例如 `math::float3`），因此脚本系统提供了 mapping 导出概念，以 javascript 为例，float3 导出的对象等同于如下代码：
```javascript
let vec = {
    x: 0.0,
    y: 0.0,
    z: 0.0,
}
```
这样，对象只需要进行一次内存分配，且访问不需要经过 C++ 代码，更便于触发一些脚本自身的优化途径

### Object（对象导出）
主要用于 `ScriptbleObject` 子类的导出，与 CPP 之间能进行相对良好的生命周期交互，通常情况下，所有权由 `EScriptbleObjectOwnership` 控制，有且仅有脚本完全持有对象所有权时，脚本才允许对对象进行销毁。当 cpp 对象销毁时，会通知脚本系统，脚本系统会将该对象对应的脚本对象标记为死亡，任何操作该对象的行为都会在脚本中引发 Exception，而不是在 C++ 中引发内存错误。

### Value（值导出）
与 Object 类型类似，区别在于不继承于 `ScriptbleObject`，因此任何对于不明确归属的指针持有是危险的，通常这类导出都会在脚本侧拷贝一份副本，跟随脚本对象的生命周期，用以保证内存的安全性，除了没法使用 mixin 之外，其功能与 Object 导出基本一致。

## 导出行为表

> 注：(copy) 代表脚本侧会拷贝一个副本，(by ref) 代表脚本侧会持有数据的指针

### 参数（Script 调用 Native）
|          | primitive |  enum   | mapping |   value   |   object   |
| :------: | :-------: | :-----: | :-----: | :-------: | :--------: |
|    T     |  (copy)T  | (copy)T | (copy)T |  (copy)T  |     -      |
|    T*    |     -     |    -    |    -    |     -     | (by ref)T? |
| const T* |     -     |    -    |    -    |     -     | (by ref)T? |
|    T&    |  (copy)T  | (copy)T | (copy)T | (by ref)T |     -      |
| const T& |  (copy)T  | (copy)T | (copy)T | (by ref)T |     -      |

### 参数（Native 调用 Script）
|          | primitive |  enum   | mapping |   value   |   object   |
| :------: | :-------: | :-----: | :-----: | :-------: | :--------: |
|    T     |  (copy)T  | (copy)T | (copy)T | (by ref)T |     -      |
|    T*    |     -     |    -    |    -    |     -     | (by ref)T? |
| const T* |     -     |    -    |    -    |     -     | (by ref)T? |
|    T&    |  (copy)T  | (copy)T | (copy)T | (by ref)T |     -      |
| const T& |  (copy)T  | (copy)T | (copy)T | (by ref)T |     -      |

### 返回值（Script 调用 Native）
|          | primitive |  enum   | mapping |  value  |   object   |
| :------: | :-------: | :-----: | :-----: | :-----: | :--------: |
|    T     |  (copy)T  | (copy)T | (copy)T | (copy)T |     -      |
|    T*    |     -     |    -    |    -    |    -    | (by ref)T? |
| const T* |     -     |    -    |    -    |    -    | (by ref)T? |
|    T&    |  (copy)T  | (copy)T | (copy)T | (copy)T |     -      |
| const T& |  (copy)T  | (copy)T | (copy)T | (copy)T |     -      |

### 返回值（Native 调用 Script）
|          | primitive |  enum   | mapping |  value  |   object   |
| :------: | :-------: | :-----: | :-----: | :-----: | :--------: |
|    T     |  (copy)T  | (copy)T | (copy)T | (copy)T |     -      |
|    T*    |     -     |    -    |    -    |    -    | (by ref)T? |
| const T* |     -     |    -    |    -    |    -    | (by ref)T? |
|    T&    |     -     |    -    |    -    |    -    |     -      |
| const T& |     -     |    -    |    -    |    -    |     -      |

### 成员变量（Script 调用 Native）
|          | primitive |  enum   | mapping |   value   |   object   |
| :------: | :-------: | :-----: | :-----: | :-------: | :--------: |
|    T     |  (copy)T  | (copy)T | (copy)T | (by ref)T |     -      |
|    T*    |     -     |    -    |    -    |     -     | (by ref)T? |
| const T* |     -     |    -    |    -    |     -     | (by ref)T? |
| const T& |     -     |    -    |    -    |     -     |     -      |
|    T&    |     -     |    -    |    -    |     -     |     -      |

### Value 导出优化
Value 导出在一些特殊情况下可以进行优化
- Field：持有对应的指针，当 CPP 对象被销毁时，对应的 Field 导出对象也会被标记为死亡
- Param：在 Native 调用 Script 时，作为参数的 Value 对象可以导出为直接持有其指针的对象，脱离函数作用域之后，该对象将会自动失效，任意使用行为都会引发 Exception

## 一些杂项
### 关于 Enum 的导出
Enum 导出的类型依赖于其 Underling Type，通常作为整数使用，同时导出时会提供 String 和整数之间的互相转化以方便使用

### 关于继承与函数重载
跨域继承是***不允许***的，脚本重载 C++ 逻辑的行为我们一般称之为 Mixin，Mixin 函数一般由一个代码生成的胶水函数，以及一个 C++ 的默认实现组成，e.g:
```cpp
struct MyMixin : ScriptableObject {
    //...
    sscript_visible sscript_mixin
    int32_t add(int32_t a, int32_t b);
    int32_t add_impl(int32_t a, int32_t b) {
        return a + b;
    }
};
```
在脚本侧，可以通过继承/直接 assign 函数等形式对这个逻辑进行重载，以 javascript 为例：
```javascript
//========= way 1
class JSMixin extends MyMixin {
    add(a, b) {
        return a + b + 1; // 重载逻辑
    }
}
let mixin = new JSMixin(); // 该对象的 mixin 行为由 JSMixin 实现

//========= way 2
let mixin = new MyMixin();
let old_add = mixin.add; // 保存原有逻辑
mixin.add = function(a, b) {
    return old_add.call(this, a, b) + 1; // 重载逻辑
};
```

### 关于 In/Out/InOut 在脚本的表现
- In：正常行为，出现在参数中
- Out：不会出现在参数中，但是会出现在返回值中
- InOut：同时出现在参数和返回值中，需要注意的是 Value 类型由于特殊优化，在函数体内可以直接操作原始对象的指针，因此不会出现在返回值中

### 关于 StringView
主要是为了便于 C++ 编码，脚本侧有时会获得更好的性能，但是有以下限制：
- 不能作为引用传递
- 不能用作为 Field
- 不能出现在返回值中