
'''
Enum json structure
<enum_name: str> : {
    "attrs": <user attributes: Object>,
    "values": {
        <enum_value_name> : {
            "attrs": <user attributes: Object>,
            "value": <value: int>,
            "comment": <comment: str>,
            "line": <line: int>,
        },
        ...
    },
    "isScoped": <is_scoped: bool>,
    "underlyingType": <underlying_type: str>,
    "comment": <comment: str>,
    "fileName": <file_name: str>
    "line": <line: int>
}

Record json structure
<record_name: str> : {
    "bases": <bases: List[str]>,
    "attrs": <user attributes: Object>,
    "fields": <fields: Dict[Fields]>,
    "methods": <methods: List[Methods]>,
    "comment": <comment: str>,
    "fileName": <file_name: str>
    "line": <line: int>
}

Field json structure
<field_name> : {
    "type": <type: str>,
    "rawType": <raw_type: str>,
    "arraySize": <array_size: int>,
    "attrs": <user attributes: Object>,
    "access": <access: str>,
    "isFunctor": <is_functor: bool>,
    "isStatic": <is_static: bool>,
    "isAnonymous": <is_anonymous: bool>,
    "comment": <comment: str>,
    "line": <line: int>,
}

Method json structure
{
    "name": <name: str>,
    "isStatic": <is_static: bool>,
    "isConst": <is_const: bool>,
    "isNothrow": <is_nothrow: bool>,
    "attrs": <user attributes: Object>,
    "access": <access: str>,
    "default_value": <default_value: str>,
    "comment": <comment: str>,
    "parameters": {
        <name: str>: {
            "type": <type: str>,
            "arraySize": <array_size: int>,
            "rawType": <raw_type: str>,
            "attrs": <user attributes: Object>,
            "isFunctor": <is_functor: bool>,
            "isCallback": <is_callback: bool>,
            "isAnonymous": <is_anonymous: bool>,
            "comment": <comment: str>,
            "line": <line: int>,
            "functor": <functor: Function>
        },
        ...
    },
    "retType": <return_type: str>,
    "rawRetType": <raw_return_type: str>,
    "line": <line: int>,
}

Parameter json structure
<name: str>: {
    "type": <type: str>,
    "arraySize": <array_size: int>,
    "rawType": <raw_type: str>,
    "attrs": <user attributes: Object>,
    "default_value": <default_value: str>,
    "isFunctor": <is_functor: bool>,
    "isCallback": <is_callback: bool>,
    "isAnonymous": <is_anonymous: bool>,
    "comment": <comment: str>,
    "line": <line: int>,
}

Function json structure
{
    "name": <name: str>,
    "isStatic": <is_static: bool>,
    "isConst": <is_const: bool>,
    "attrs": <user attributes: Object>,
    "comment": <comment: str>,
    "parameters": {
        <parameter_name> : {
            "type": <type: str>,
            "arraySize": <array_size: int>,
            "rawType": <raw_type: str>,
            "attrs": <user attributes: Object>,
            "isFunctor": <is_functor: bool>,
            "isCallback": <is_callback: bool>,
            "isAnonymous": <is_anonymous: bool>,
            "comment": <comment: str>,
            "line": <line: int>,
            "functor": <functor: Function>
        },
        ...
    },
    "retType": <return_type: str>,
    "rawRetType": <raw_return_type: str>,
    "fileName": <file_name: str>
    "line": <line: int>
    }
}
'''
# TODO. 需要一个签名器来直接描述一个类型的签名
#       如果是 func 则需要递归 (需要 params 的 attr 和 params name) 否则对齐 cpp 的签名
#       为了支持这个签名器, 还需要一个专门的 scheme 目标类型来解析
#       https://stackoverflow.com/questions/53910964/how-to-get-function-pointer-arguments-names-using-clang-libtooling
#
# TODO. name/attrs 等解析可以做成带 cache 的现场解析，避免过多冗余逻辑提升性能
from typing import List, Dict
import framework.scheme as sc
import framework.log as log


class EnumerationValue:
    def __init__(self, name, parent) -> None:
        self.parent: 'Enumeration' = parent

        split_name = str.rsplit(name, "::", 1)
        self.name: str = name
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.value: int
        self.comment: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.value = unique_dict["value"]
        self.comment = unique_dict["comment"]
        self.line = unique_dict["line"]

        # load fields
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.parent.file_name, self.line)


class Enumeration:
    def __init__(self, name) -> None:
        split_name = str.rsplit(name, "::", 1)
        self.name: str = name
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.values: Dict[str, EnumerationValue]
        self.is_scoped: bool
        self.underlying_type: str
        self.comment: str
        self.file_name: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.is_scoped = unique_dict["isScoped"]
        self.underlying_type = unique_dict["underlying_type"]
        self.comment = unique_dict["comment"]
        self.file_name = unique_dict["fileName"]
        self.line = unique_dict["line"]

        # load values
        self.values = {}
        for (enum_value_name, enum_value_data) in unique_dict["values"].unique_dict().items():
            value = EnumerationValue(enum_value_name, self)
            value.load_from_raw_json(enum_value_data)
            self.values[enum_value_name] = value

        # load attrs
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.file_name, self.line)


class Record:
    def __init__(self, name) -> None:
        split_name = str.rsplit(name, "::", 1)
        self.name: str = name
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.bases: List[str]
        self.fields: List[Field]
        self.methods: List[Method]
        self.has_generate_body_flag: bool = False
        self.generate_body_line: int = 0
        self.generated_body_content: str = ""
        self.file_name: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.bases = unique_dict["bases"]
        self.comment = unique_dict["comment"]
        self.file_name = unique_dict["fileName"]
        self.line = unique_dict["line"]

        # load fields
        self.fields = []
        for (field_name, field_data) in unique_dict["fields"].unique_dict().items():
            field = Field(field_name, self)
            field.load_from_raw_json(field_data)
            self.fields.append(field)

        # load methods
        self.methods = []
        for method_data in unique_dict["methods"]:
            method = Method(self)
            method.load_from_raw_json(method_data)
            if method.short_name == "_zz_skr_generate_body_flag":
                self.has_generate_body_flag = True
                self.generate_body_line = method.line
            else:
                self.methods.append(method)

        # load attrs
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def dump_generate_body_content(self):
        result = ""
        lines = self.generated_body_content.splitlines()
        for line in lines:
            result += f"{line} \\\n"
        return result

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.file_name, self.line)


class Field:
    def __init__(self, name, parent) -> None:
        self.parent: Record = parent

        self.name: str = name

        self.type: str
        self.raw_type: str
        self.access: str
        self.default_value: str
        self.array_size: int
        self.is_functor: bool
        self.is_static: bool
        self.is_anonymous: bool
        self.comment: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.type = unique_dict["type"]
        self.raw_type = unique_dict["rawType"]
        self.access = unique_dict["access"]
        self.default_value = unique_dict["defaultValue"]
        self.array_size = unique_dict["arraySize"]
        self.is_functor = unique_dict["isFunctor"]
        self.is_anonymous = unique_dict["isAnonymous"]
        self.is_static = unique_dict["isStatic"]
        self.comment = unique_dict["comment"]
        self.line = unique_dict["line"]

        # load attrs
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.parent.file_name, self.line)


class Method:
    def __init__(self, parent) -> None:
        self.parent: Record = parent

        self.name: str
        self.short_name: str
        self.namespace: str

        self.access: str
        self.is_static: bool
        self.is_const: bool
        self.is_nothrow: bool
        self.comment: str
        self.parameters: Dict[str, Parameter]
        self.ret_type: str
        self.raw_ret_type: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.name = unique_dict["name"]
        split_name = str.rsplit(self.name, "::", 1)
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.access = unique_dict["access"]
        self.is_static = unique_dict["isStatic"]
        self.is_const = unique_dict["isConst"]
        self.is_nothrow = unique_dict["isNothrow"]
        self.comment = unique_dict["comment"]
        self.ret_type = unique_dict["retType"]
        self.raw_ret_type = unique_dict["rawRetType"]
        self.line = unique_dict["line"]

        # load parameters
        self.parameters = {}
        for (param_name, param_data) in unique_dict["parameters"].unique_dict().items():
            param = Parameter(param_name, self)
            param.load_from_raw_json(param_data)
            self.parameters[param_name] = param

        # load attrs
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.parent.file_name, self.line)

    def dump_params(self):
        return ", ".join([f"{param.type} {param.name}" for param in self.parameters.values()])

    def dump_params_with_comma(self):
        param_expr = self.dump_params()
        return f", {param_expr}" if param_expr else ""

    def dump_params_name_only(self):
        return ", ".join([param.name for param in self.parameters.values()])

    def dump_params_name_only_with_comma(self):
        param_expr = self.dump_params_name_only()
        return f", {param_expr}" if param_expr else ""

    def dump_const(self):
        return "const" if self.is_const else ""

    def dump_noexcept(self):
        return "noexcept" if self.is_nothrow else ""


class Parameter:
    def __init__(self, name, parent) -> None:
        self.parent: Method | 'Function' = parent

        self.name: str = name
        self.type: str
        self.array_size: int
        self.raw_type: str
        self.is_functor: bool
        self.is_callback: bool
        self.is_anonymous: bool
        self.functor: 'Function'
        self.default_value: str
        self.comment: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.type = unique_dict["type"]
        self.array_size = unique_dict["arraySize"]
        self.raw_type = unique_dict["rawType"]
        self.is_functor = unique_dict["isFunctor"]
        self.is_anonymous = unique_dict["isAnonymous"]
        self.comment = unique_dict["comment"]
        self.line = unique_dict["line"]
        self.default_value = unique_dict["defaultValue"]

        # TODO. load functor

        # load attrs
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        if isinstance(self.parent, Method):
            return log.CppSourceStack(self.parent.parent.file_name, self.line)
        else:
            return log.CppSourceStack(self.parent.file_name, self.line)


class Function:
    def __init__(self) -> None:
        self.name: str
        self.short_name: str
        self.namespace: str

        self.is_static: bool
        self.is_const: bool
        self.comment: str
        self.parameters: Dict[str, Parameter]
        self.ret_type: str
        self.raw_ret_type: str
        self.file_name: str
        self.line: int

        self.raw_attrs: sc.JsonObject
        self.attrs: sc.ParseResult
        self.generator_data: Dict[str, object] = {}

    def load_from_raw_json(self, raw_json: sc.JsonObject):
        unique_dict = raw_json.unique_dict()

        self.name = unique_dict["name"]
        split_name = str.rsplit(self.name, "::", 1)
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.is_static = unique_dict["isStatic"]
        self.is_const = unique_dict["isConst"]
        self.comment = unique_dict["comment"]
        self.ret_type = unique_dict["retType"]
        self.raw_ret_type = unique_dict["rawRetType"]
        self.file_name = unique_dict["fileName"]
        self.line = unique_dict["line"]

        # load parameters
        self.parameters = {}
        for (param_name, param_data) in unique_dict["parameters"].unique_dict().items():
            param = Parameter(param_name, self)
            param.load_from_raw_json(param_data)
            self.parameters[param_name] = param

        # load attrs
        self.raw_attrs = unique_dict["attrs"]
        self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.file_name, self.line)
