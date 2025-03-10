import * as gen from "@framework/generator"
import * as db from "@framework/database"
import * as ml from "@framework/meta_lang"

class ConfigBase extends ml.WithEnable {
  @ml.array('string')
  flags: string[] = []

  @ml.value('string')
  attrs: string = ""
}

class GuidConfig {
  @ml.value('string')
  value: string = ""

  @ml.value_proxy('string')
  proxy_value(v: string) {
    this.value = v;
  }

  toString() { return this.value; }
}

class RecordConfig extends ConfigBase {
  // default is true 
  override enable: boolean = true

  @ml.value('boolean')
  reflect_bases: boolean = true

  @ml.array('string')
  exclude_bases: string[] = []

  @ml.value('boolean')
  reflect_fields: boolean = false

  @ml.value('boolean')
  reflect_methods: boolean = false

  @ml.preset('full')
  preset_full() {
    this.reflect_bases = true;
    this.reflect_fields = true;
    this.reflect_methods = true;
  }

  @ml.preset('fields')
  preset_fields() {
    this.reflect_fields = true;
  }

  @ml.preset('methods')
  preset_methods() {
    this.reflect_methods = true;
  }

  @ml.preset('minimal')
  preset_minimal() {
    this.reflect_bases = false;
    this.reflect_fields = false;
    this.reflect_methods = false;
  }
}
class FieldConfig extends ConfigBase {
}
class MethodConfig extends ConfigBase {
}
class ParamConfig extends ConfigBase {
}
class EnumConfig extends ConfigBase {
}
class EnumValueConfig extends ConfigBase {
}

class _Gen {
  static body(record: db.Record) {
    record.generate_body_content.block(`
::skr::GUID iobject_get_typeid() const override
{
    using namespace skr::rttr;
    using ThisType = std::remove_cv_t<std::remove_pointer_t<decltype(this)>>;
    return type_id_of<ThisType>();
}
void* iobject_get_head_ptr() const override { return const_cast<void*>((const void*)this); }
`)
  }
  static header(header: db.Header) { }
  static source(main_db: db.Module) { }
}

class RttrGenerator extends gen.Generator {
  gen_body(): void {
  }
  gen(): void {
  }
}

export function load_generator(gm: gen.GenerateManager) {
  gm.add_generator("rttr", new RttrGenerator())
}