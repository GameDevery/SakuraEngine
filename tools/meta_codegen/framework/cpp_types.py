# TODO. 需要一个签名器来直接描述一个类型的签名
#       如果是 func 则需要递归 (需要 params 的 attr 和 params name) 否则对齐 cpp 的签名
#       为了支持这个签名器, 还需要一个专门的 scheme 目标类型来解析
#       https://stackoverflow.com/questions/53910964/how-to-get-function-pointer-arguments-names-using-clang-libtooling
#
# TODO. name/attrs 等解析可以做成带 cache 的现场解析，避免过多冗余逻辑提升性能
from typing import List, Dict
import framework.scheme as sc
import framework.log as log
import json

def parse_attrs(attrs: List[str]):
    result = sc.JsonObject(
        val=[],
        is_dict=True,
    )
    for attr in attrs:
        try:
            json_obj = json.loads(attr, object_pairs_hook=sc.json_object_pairs_hook)
        except json.JSONDecodeError as e:
            raise Exception(f"failed to parse attr: {attr}\n{e}")
        
        for v in json_obj.val:
            result.val.append(v)
    return result


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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.value = unique_dict["value"]
        self.comment = unique_dict["comment"]
        self.line = unique_dict["line"]

        # load fields
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.is_scoped = unique_dict["is_scoped"]
        self.underlying_type = unique_dict["underlying_type"]
        self.comment = unique_dict["comment"]
        self.file_name = unique_dict["file_name"]
        self.line = unique_dict["line"]

        # load values
        self.values = {}
        for enum_value_data in unique_dict["values"]:
            value = EnumerationValue(enum_value_data["name"], self)
            value.load_from_raw_json(enum_value_data)
            self.values[enum_value_data["name"]] = value

        # load attrs
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.bases = unique_dict["bases"]
        self.comment = unique_dict["comment"]
        self.file_name = unique_dict["file_name"]
        self.line = unique_dict["line"]

        # load fields
        self.fields = []
        for field_data in unique_dict["fields"]:
            field = Field(field_data["name"], self)
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
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.type = unique_dict["type"]
        self.raw_type = unique_dict["raw_type"]
        self.access = unique_dict["access"]
        self.default_value = unique_dict["default_value"]
        self.array_size = unique_dict["array_size"]
        self.is_functor = unique_dict["is_functor"]
        self.is_anonymous = unique_dict["is_anonymous"]
        self.is_static = unique_dict["is_static"]
        self.comment = unique_dict["comment"]
        self.line = unique_dict["line"]

        # load attrs
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.name = unique_dict["name"]
        split_name = str.rsplit(self.name, "::", 1)
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.access = unique_dict["access"]
        self.is_static = unique_dict["is_static"]
        self.is_const = unique_dict["is_const"]
        self.is_nothrow = unique_dict["is_nothrow"]
        self.comment = unique_dict["comment"]
        self.ret_type = unique_dict["ret_type"]
        self.raw_ret_type = unique_dict["raw_ret_type"]
        self.line = unique_dict["line"]

        # load parameters
        self.parameters = {}
        for param_data in unique_dict["parameters"]:
            param = Parameter(param_data["name"], self)
            param.load_from_raw_json(param_data)
            self.parameters[param_data["name"]] = param

        # load attrs
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.type = unique_dict["type"]
        self.array_size = unique_dict["array_size"]
        self.raw_type = unique_dict["raw_type"]
        self.is_functor = unique_dict["is_functor"]
        self.is_anonymous = unique_dict["is_anonymous"]
        self.comment = unique_dict["comment"]
        self.line = unique_dict["line"]
        self.default_value = unique_dict["default_value"]

        # TODO. load functor

        # load attrs
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

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

    def load_from_raw_json(self, raw_json: Dict):
        unique_dict = raw_json

        self.name = unique_dict["name"]
        split_name = str.rsplit(self.name, "::", 1)
        self.short_name: str = split_name[-1]
        self.namespace: str = split_name[0] if len(split_name) > 1 else ""

        self.is_static = unique_dict["is_static"]
        self.is_const = unique_dict["is_const"]
        self.comment = unique_dict["comment"]
        self.ret_type = unique_dict["ret_type"]
        self.raw_ret_type = unique_dict["raw_ret_type"]
        self.file_name = unique_dict["file_name"]
        self.line = unique_dict["line"]

        # load parameters
        self.parameters = {}
        for (param_name, param_data) in unique_dict["parameters"].items():
            param = Parameter(param_name, self)
            param.load_from_raw_json(param_data)
            self.parameters[param_name] = param

        # load attrs
        self.raw_attrs = parse_attrs(unique_dict["attrs"])
        # self.raw_attrs.escape_from_parent()

    def make_log_stack(self) -> log.CppSourceStack:
        return log.CppSourceStack(self.file_name, self.line)
