import {
  CodeBuilder, type_convert_options,
  type_options, all_component_kinds,
  dims_all, dims_no_scalar, vector_has_simd_optimize
} from "./util"
import type { TypeOption, ComponentKind, GlobalBuilders } from "./util";
import path from "node:path";

const _comp_lut = ['x', 'y', 'z', 'w'];

interface MathGenOptions extends TypeOption {
  base_name: string;
  dim: number;
}

type _MathGenFunc = (b: CodeBuilder, opt: MathGenOptions) => void;
type _MathGenFuncConfig = {
  name: string
  func: _MathGenFunc
  accept_comp_kind: ComponentKind[]
  accept_dim: number[]
}
const _math_gen_config_symbol = Symbol('math_gen_config');
function math_func(name: string, opt: {
  accept_comp_kind?: ComponentKind[],
  accept_dim?: number[],
} = {}
) {
  return (target: any, method_name: string) => {
    let config_dict = (target[_math_gen_config_symbol] ??= {}) as Dict<_MathGenFuncConfig>;

    config_dict[`${method_name}`] = {
      name: name,
      func: target[method_name],
      accept_comp_kind: opt.accept_comp_kind ?? all_component_kinds,
      accept_dim: opt.accept_dim ?? dims_all,
    }
  }
}


class _MathFuncGenerator {
  //=====================call std=====================
  static #call_std_func(
    name: string,
    std_name: string,
    b: CodeBuilder,
    opt: MathGenOptions,
    param_names: string[],
    override_result_type?: string,
  ) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;
    const comp_name = opt.component_name;
    const return_vec_name = override_result_type ? `${override_result_type}${dim}` : vec_name;

    if (vector_has_simd_optimize(name, opt, dim)) {
      if (dim === 1) {
        const return_comp_name = override_result_type ? override_result_type : comp_name;
        const param_list = param_names
          .map(p => `${comp_name} ${p}`)
          .join(', ')

        b.$line(`${return_comp_name} ${name}(${param_list});`)
      } else {
        const param_list = param_names
          .map(p => `const ${vec_name}& ${p}`)
          .join(', ')

        b.$line(`${return_vec_name} ${name}(${param_list});`)
      }

    } else {
      if (dim === 1) {
        const return_comp_name = override_result_type ? override_result_type : comp_name;

        const param_list = param_names
          .map(p => `${comp_name} ${p}`)
          .join(', ')

        const param_expr = param_names
          .join(', ')

        b.$line(`inline ${return_comp_name} ${name}(${param_list}) { return ::std::${std_name}(${param_expr}); }`)
      } else {
        const param_list = param_names
          .map(p => `const ${vec_name}& ${p}`)
          .join(', ')

        const init_expr = _comp_lut
          .slice(0, dim)
          .map(c => {
            const param_list = param_names
              .map(p => `${p}.${c}`)
              .join(', ')
            return `::std::${std_name}(${param_list})`
          })
          .join(', ');

        b.$line(`inline ${return_vec_name} ${name}(${param_list}) { return {${init_expr}}; }`)
      }
    }
  }

  @math_func("abs", { accept_comp_kind: ["floating", "integer"] })
  static gen_abs(b: CodeBuilder, opt: MathGenOptions) {
    if (opt.component_kind === "integer") {
      const base_name = opt.base_name;
      const dim = opt.dim;
      const vec_name = `${base_name}${dim}`;
      const comp_name = opt.component_name;
      const return_vec_name = `${base_name}${dim}`;

      if (dim === 1) {
        const init_expr = `v < ${comp_name}(0) ? -v : v`;
        b.$line(`inline ${comp_name} abs(${comp_name} v) { return ${init_expr}; }`)
      } else {
        const init_expr = _comp_lut
          .slice(0, dim)
          .map(c => `abs(v.${c})`)
          .join(', ');

        b.$line(`inline ${return_vec_name} abs(const ${vec_name}& v) { return {${init_expr}}; }`)
      }

    } else {
      this.#call_std_func("abs", "abs", b, opt, ["v"]);
    }
  }

  // cos
  @math_func("acos", { accept_comp_kind: ["floating"] })
  static gen_acos(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("acos", "acos", b, opt, ["v"]);
  }
  @math_func("acosh", { accept_comp_kind: ["floating"] })
  static gen_acosh(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("acosh", "acosh", b, opt, ["v"]);
  }
  @math_func("cos", { accept_comp_kind: ["floating"] })
  static gen_cos(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("cos", "cos", b, opt, ["v"]);
  }
  @math_func("cosh", { accept_comp_kind: ["floating"] })
  static gen_cosh(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("cosh", "cosh", b, opt, ["v"]);
  }

  // sin
  @math_func("asin", { accept_comp_kind: ["floating"] })
  static gen_asin(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("asin", "asin", b, opt, ["v"]);
  }
  @math_func("asinh", { accept_comp_kind: ["floating"] })
  static gen_asinh(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("asinh", "asinh", b, opt, ["v"]);
  }
  @math_func("sin", { accept_comp_kind: ["floating"] })
  static gen_sin(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("sin", "sin", b, opt, ["v"]);
  }
  @math_func("sinh", { accept_comp_kind: ["floating"] })
  static gen_sinh(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("sinh", "sinh", b, opt, ["v"]);
  }

  // tan
  @math_func("atan", { accept_comp_kind: ["floating"] })
  static gen_atan(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("atan", "atan", b, opt, ["v"]);
  }
  @math_func("atanh", { accept_comp_kind: ["floating"] })
  static gen_atanh(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("atanh", "atanh", b, opt, ["v"]);
  }
  @math_func("tan", { accept_comp_kind: ["floating"] })
  static gen_tan(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("tan", "tan", b, opt, ["v"]);
  }
  @math_func("tanh", { accept_comp_kind: ["floating"] })
  static gen_tanh(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("tanh", "tanh", b, opt, ["v"]);
  }

  // sincos
  @math_func("sincos", { accept_comp_kind: ["floating"] })
  static gen_sincos(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      b.$line(`inline void sincos(${comp_name} v, ${comp_name}& out_sin, ${comp_name}& out_cos) { out_sin = ::std::sin(v); out_cos = ::std::cos(v); }`)
      return;
    }
    else {
      // use sincos
      const exprs = _comp_lut
        .slice(0, dim)
        .map(c => `sincos(v.${c}, out_sin.${c}, out_cos.${c});`)
        .join(' ');
      b.$line(`inline void sincos(const ${vec_name}& v, ${vec_name}& out_sin, ${vec_name}& out_cos) { ${exprs} }`)
    }
  }


  // tan2
  @math_func("atan2", { accept_comp_kind: ["floating"] })
  static gen_atan2(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("atan2", "atan2", b, opt, ["y", "x"]);
  }

  // ceil & floor & round & trunc & frac
  @math_func("ceil", { accept_comp_kind: ["floating"] })
  static gen_ceil(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("ceil", "ceil", b, opt, ["v"]);
  }
  @math_func("floor", { accept_comp_kind: ["floating"] })
  static gen_floor(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("floor", "floor", b, opt, ["v"]);
  }
  @math_func("round", { accept_comp_kind: ["floating"] })
  static gen_round(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("round", "round", b, opt, ["v"]);
  }
  @math_func("trunc", { accept_comp_kind: ["floating"] })
  static gen_trunc(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("trunc", "trunc", b, opt, ["v"]);
  }
  @math_func("frac", { accept_comp_kind: ["floating"] })
  static gen_frac(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const int_ptr = `${comp_name} int_ptr`;
      const init_expr = `::std::modf(v, &int_ptr)`;
      b.$line(`inline ${comp_name} frac(${comp_name} v) { ${int_ptr}; return ${init_expr}; }`)
    } else {
      // use modf
      const int_ptr_vec = `${base_name}${dim} int_ptr`;
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `::std::modf(v.${c}, &int_ptr.${c})`)
        .join(', ');

      b.$line(`inline ${vec_name} frac(const ${vec_name}& v) { ${int_ptr_vec}; return {${init_expr}}; }`)
    }
  }
  @math_func("modf", { accept_comp_kind: ["floating"] })
  static gen_modf(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `::std::modf(v, &int_part)`;
      b.$line(`inline ${comp_name} modf(${comp_name} v, ${comp_name}& int_part) { return ${init_expr}; }`)
    } else {
      // use modf
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `::std::modf(v.${c}, &int_part.${c})`)
        .join(', ');

      b.$line(`inline ${vec_name} modf(const ${vec_name}& v, ${vec_name}& int_part) { return { ${init_expr} }; }`)
    }
  }

  // fmod
  @math_func("fmod", { accept_comp_kind: ["floating"] })
  static gen_fmod(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("fmod", "fmod", b, opt, ["x", "y"]);
  }

  // exp & log
  @math_func("exp", { accept_comp_kind: ["floating"] })
  static gen_exp(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("exp", "exp", b, opt, ["v"]);
  }
  @math_func("exp2", { accept_comp_kind: ["floating"] })
  static gen_exp2(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("exp2", "exp2", b, opt, ["v"]);
  }
  @math_func("log", { accept_comp_kind: ["floating"] })
  static gen_log(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("log", "log", b, opt, ["v"]);
  }
  @math_func("log2", { accept_comp_kind: ["floating"] })
  static gen_log2(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("log2", "log2", b, opt, ["v"]);
  }
  @math_func("log10", { accept_comp_kind: ["floating"] })
  static gen_log10(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("log10", "log10", b, opt, ["v"]);
  }
  @math_func("logx", { accept_comp_kind: ["floating"] })
  static gen_logx(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      // use ::std::log
      const init_expr = `::std::log(v) / ::std::log(base)`;
      b.$line(`inline ${comp_name} logx(${comp_name} v, ${comp_name} base) { return ${init_expr}; }`)
    } else {
      // use logx
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `logx(v.${c}, base.${c})`)
        .join(', ');
      b.$line(`inline ${vec_name} logx(const ${vec_name}& v, const ${vec_name}& base) { return {${init_expr}}; }`)
    }

  }

  // isfinite & isinf & isnan
  @math_func("isfinite", { accept_comp_kind: ["floating"] })
  static gen_isfinite(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("isfinite", "isfinite", b, opt, ["v"], "bool");
  }
  @math_func("isinf", { accept_comp_kind: ["floating"] })
  static gen_isinf(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("isinf", "isinf", b, opt, ["v"], "bool");
  }
  @math_func("isnan", { accept_comp_kind: ["floating"] })
  static gen_isnan(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("isnan", "isnan", b, opt, ["v"], "bool");
  }

  // max & min
  @math_func("max", { accept_comp_kind: ["floating", "integer"] })
  static gen_max(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("max", "max", b, opt, ["v1", "v2"]);
  }
  @math_func("min", { accept_comp_kind: ["floating", "integer"] })
  static gen_min(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("min", "min", b, opt, ["v1", "v2"]);
  }

  // mad
  @math_func("mad", { accept_comp_kind: ["floating"] })
  static gen_mad(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("mad", "fma", b, opt, ["x", "mul", "add"]);
  }

  // pow & sqrt
  @math_func("pow", { accept_comp_kind: ["floating"] })
  static gen_pow(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("pow", "pow", b, opt, ["x", "y"]);
  }
  @math_func("sqrt", { accept_comp_kind: ["floating"] })
  static gen_sqrt(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("sqrt", "sqrt", b, opt, ["v"]);
  }
  @math_func("rsqrt", { accept_comp_kind: ["floating"] })
  static gen_rsqrt(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    if (vector_has_simd_optimize("rsqrt", opt, dim)) {
      if (dim === 1) {
        b.$line(`${comp_name} rsqrt(${comp_name} v);`)
      } else {
        b.$line(`${vec_name} rsqrt(const ${vec_name}& v);`)
      }
    } else {
      if (dim === 1) {
        const init_expr = `${comp_name}(1) / ::std::sqrt(v)`;
        b.$line(`inline ${comp_name} rsqrt(${comp_name} v) { return ${init_expr}; }`)
      } else {
        // use sqrt
        const init_expr = _comp_lut
          .slice(0, dim)
          .map(c => `${comp_name}(1) / ::std::sqrt(v.${c})`)
          .join(', ');

        b.$line(`inline ${vec_name} rsqrt(const ${vec_name}& v) { return {${init_expr}}; }`)
      }
    }
  }

  // cbrt & hypot
  @math_func("cbrt", { accept_comp_kind: ["floating"] })
  static gen_cbrt(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("cbrt", "cbrt", b, opt, ["v"]);
  }
  @math_func("hypot", { accept_comp_kind: ["floating"] })
  static gen_hypot(b: CodeBuilder, opt: MathGenOptions) {
    this.#call_std_func("hypot", "hypot", b, opt, ["x", "y"]);
    this.#call_std_func("hypot", "hypot", b, opt, ["x", "y", "z"]);
  }


  //=====================simple functions=====================
  // clamp & saturate
  @math_func("clamp", { accept_comp_kind: ["floating", "integer"] })
  static gen_clamp(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v < min ? min : v > max ? max : v`;
      b.$line(`inline ${comp_name} clamp(${comp_name} v, ${comp_name} min, ${comp_name} max) { return ${init_expr}; }`)
    } else {
      // use clamp
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `clamp(v.${c}, min.${c}, max.${c})`)
        .join(', ');

      b.$line(`inline ${vec_name} clamp(const ${vec_name}& v, const ${vec_name}& min, const ${vec_name}& max) { return {${init_expr}}; }`)
    }
  }
  @math_func("saturate", { accept_comp_kind: ["floating"] })
  static gen_saturate(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `clamp(v, ${comp_name}(0), ${comp_name}(1))`;
      b.$line(`inline ${comp_name} saturate(${comp_name} v) { return ${init_expr}; }`)
    } else {
      // use clamp
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `clamp(v.${c}, ${comp_name}(0), ${comp_name}(1))`)
        .join(', ');

      b.$line(`inline ${vec_name} saturate(const ${vec_name}& v) { return {${init_expr}}; }`)
    }
  }

  @math_func("select", { accept_dim: dims_no_scalar })
  static gen_select(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    const select_expr = _comp_lut
      .slice(0, dim)
      .map(c => `c.${c} ? if_true.${c} : if_false.${c}`)
      .join(', ');

    b.$line(`inline ${vec_name} select(bool${dim} c, ${vec_name} if_true, ${vec_name} if_false) { return { ${select_expr} }; }`);
  }

  // all & any
  @math_func("all", { accept_comp_kind: ["boolean"], accept_dim: dims_no_scalar })
  static gen_all(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    const compare_expr = _comp_lut
      .slice(0, dim)
      .map(c => `v.${c}`)
      .join(' && ');

    b.$line(`inline bool all(const ${vec_name}& v) { return ${compare_expr}; }`);
  }
  @math_func("any", { accept_comp_kind: ["boolean"], accept_dim: dims_no_scalar })
  static gen_any(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const comp_kind = opt.component_kind;
    const dim = opt.dim;
    const vec_name = `${base_name}${dim}`;

    const compare_expr = _comp_lut
      .slice(0, dim)
      .map(c => `v.${c}`)
      .join(' || ');

    b.$line(`inline bool any(const ${vec_name}& v) { return ${compare_expr}; }`);
  }

  @math_func("rcp", { accept_comp_kind: ["floating"] })
  static gen_rcp(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (vector_has_simd_optimize("rcp", opt, dim)) {
      if (dim === 1) {
        b.$line(`${comp_name} rcp(${comp_name} v);`)
      } else {
        b.$line(`${vec_name} rcp(const ${vec_name}& v);`)
      }
    } else {
      if (dim === 1) {
        const init_expr = `${comp_name}(1) / v`;
        b.$line(`inline ${comp_name} rcp(${comp_name} v) { return ${init_expr}; }`)
      } else {
        const init_expr = _comp_lut
          .slice(0, dim)
          .map(c => `${comp_name}(1) / v.${c}`)
          .join(', ');

        b.$line(`inline ${vec_name} rcp(const ${vec_name}& v) { return {${init_expr}}; }`)
      }
    }
  }

  @math_func("sign", { accept_comp_kind: ["floating"] })
  static gen_sign(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v < ${comp_name}(0) ? ${comp_name}(-1) : v > ${comp_name}(0) ? ${comp_name}(1) : ${comp_name}(0)`;
      b.$line(`inline ${comp_name} sign(${comp_name} v) { return ${init_expr}; }`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `v.${c} < ${comp_name}(0) ? ${comp_name}(-1) : v.${c} > ${comp_name}(0) ? ${comp_name}(1) : ${comp_name}(0)`)
        .join(', ');

      b.$line(`inline ${vec_name} sign(const ${vec_name}& v) { return {${init_expr}}; }`)
    }
  }

  // deg & rad
  @math_func("degrees", { accept_comp_kind: ["floating"] })
  static gen_degrees(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v * ${comp_name}(180) / ${comp_name}(kPi)`;
      b.$line(`inline ${comp_name} degrees(${comp_name} v) { return ${init_expr}; }`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `v.${c} * ${comp_name}(180) / ${comp_name}(kPi)`)
        .join(', ');

      b.$line(`inline ${vec_name} degrees(const ${vec_name}& v) { return {${init_expr}}; }`)
    }
  }
  @math_func("radians", { accept_comp_kind: ["floating"] })
  static gen_radians(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v * ${comp_name}(kPi) / ${comp_name}(180)`;
      b.$line(`inline ${comp_name} radians(${comp_name} v) { return ${init_expr}; }`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `v.${c} * ${comp_name}(kPi) / ${comp_name}(180)`)
        .join(', ');

      b.$line(`inline ${vec_name} radians(const ${vec_name}& v) { return {${init_expr}}; }`)
    }
  }

  // cross & dot
  @math_func("dot", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_dot(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (vector_has_simd_optimize("cross", opt, dim)) {
      b.$line(`${comp_name} dot(const ${vec_name}& v1, const ${vec_name}& v2);`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `v1.${c} * v2.${c}`)
        .join(' + ');

      b.$line(`inline ${comp_name} dot(const ${vec_name}& v1, const ${vec_name}& v2) { return ${init_expr}; }`)
    }
  }
  @math_func("cross", { accept_comp_kind: ["floating"], accept_dim: [3] })
  static gen_cross(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    const init_expr = `(x * y.yzx() - x.yzx() * y).yzx()`;

    if (vector_has_simd_optimize("cross", opt, dim)) {
      b.$line(`${vec_name} cross(const ${vec_name}& x, const ${vec_name}& y);`)
    } else {
      b.$line(`inline ${vec_name} cross(const ${vec_name}& x, const ${vec_name}& y) { return {${init_expr}}; }`)
    }
  }

  // length & distance Squared
  @math_func("length", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_length(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (vector_has_simd_optimize("length", opt, dim)) {
      b.$line(`${comp_name} length(const ${vec_name}& v);`)
    } else {
      b.$line(`inline ${comp_name} length(const ${vec_name}& v) { return ::std::sqrt(dot(v, v)); }`)
    }
  }
  @math_func("length_squared", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_length_squared(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (vector_has_simd_optimize("length", opt, dim)) {
      b.$line(`${comp_name} length_squared(const ${vec_name}& v);`)
    } else {
      b.$line(`inline ${comp_name} length_squared(const ${vec_name}& v) { return dot(v, v); }`)
    }
  }
  @math_func("rlength", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_rlength(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (vector_has_simd_optimize("rlength", opt, dim)) {
      b.$line(`${comp_name} rlength(const ${vec_name}& v);`)
    } else {
      b.$line(`inline ${comp_name} rlength(const ${vec_name}& v) { return ${comp_name}(1) / length(v); }`)
    }
  }
  @math_func("distance", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_distance(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    b.$line(`inline ${comp_name} distance(const ${vec_name}& x, const ${vec_name}& y) { return length_squared(y - x); }`)
  }
  @math_func("distance_squared", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_distance_squared(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    b.$line(`inline ${comp_name} distance_squared(const ${vec_name}& x, const ${vec_name}& y) { return length(y - x); }`)
  }

  // normalize
  @math_func("normalize", { accept_comp_kind: ["floating"], accept_dim: dims_no_scalar })
  static gen_normalize(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    b.$line(`inline ${vec_name} normalize(const ${vec_name}& v) { return v / length(v); }`)
  }

  // reflect & refract
  @math_func("reflect", { accept_comp_kind: ["floating"], accept_dim: [2, 3] })
  static gen_reflect(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    b.$line(`inline ${vec_name} reflect(const ${vec_name}& v, const ${vec_name}& n) { return v - 2 * dot(n, v) * n; }`)
  }
  @math_func("refract", { accept_comp_kind: ["floating"], accept_dim: [2, 3] })
  static gen_refract(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    b.$line(`inline ${vec_name} refract(const ${vec_name}& v, const ${vec_name}& n, ${comp_name} eta) {`)
    b.$indent(_b => {
      b.$line(`const ${comp_name} cos_i = dot(-v, n);`)
      b.$line(`const ${comp_name} cos_t2 = ${comp_name}(1) - eta * eta * (${comp_name}(1) - cos_i * cos_i);`)
      b.$line(`const ${vec_name} t = eta * v + (eta * cos_i - ::std::sqrt(::std::abs(cos_t2))) * n;`)
      b.$line(`return t * ${vec_name}(cos_t2 > 0);`)
    })
    b.$line(`}`)
  }

  // step & smooth step
  @math_func("step", { accept_comp_kind: ["floating"] })
  static gen_step(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v < edge ? ${comp_name}(0) : ${comp_name}(1)`;
      b.$line(`inline ${comp_name} step(${comp_name} edge, ${comp_name} v) { return ${init_expr}; }`)
    } else {
      b.$line(`inline ${vec_name} step(const ${vec_name}& edge, const ${vec_name}& v) { return select(edge < v, ${vec_name}(0), ${vec_name}(1)); }`)
    }

  }
  @math_func("smoothstep", { accept_comp_kind: ["floating"] })
  static gen_smoothstep(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;
    const param_name = dim === 1 ? comp_name : `const ${vec_name}&`;

    b.$line(`inline ${vec_name} smoothstep(${param_name} edge0, ${param_name} edge1, ${param_name} v) {`)
    b.$indent(_b => {
      b.$line(`const ${vec_name} t = saturate((v - edge0) / (edge1 - edge0));`)
      b.$line(`return t * t * (${comp_name}(3) - ${comp_name}(2) * v);`)
    })
    b.$line(`}`)
  }

  // lerp
  @math_func("lerp", { accept_comp_kind: ["floating"] })
  static gen_lerp(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v0 + t * (v1 - v0)`;
      b.$line(`inline ${comp_name} lerp(${comp_name} v0, ${comp_name} v1, ${comp_name} t) { return ${init_expr}; }`)
    } else {
      b.$line(`inline ${vec_name} lerp(const ${vec_name}& v0, const ${vec_name}& v1, ${comp_name} t) { return v0 + t * (v1 - v0); }`)
    }
  }

  // slerp
  // @math_func("slerp", { accept_comp_kind: ["floating"], accept_dim: [2, 3] })
  // static gen_slerp(b: CodeBuilder, opt: MathGenOptions) {
  //   const base_name = opt.base_name;
  //   const dim = opt.dim;
  //   const comp_name = opt.component_name;
  //   const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

  //   b.$line(`inline ${vec_name} slerp(const ${vec_name}& v0, const ${vec_name}& v1, const ${comp_name} &t) {`)
  //   b.$indent(_b => {
  //     b.$line(`const ${comp_name} cos_theta = dot(v0, v1);`)
  //     b.$line(`const ${comp_name} theta = acos(cos_theta);`)
  //     b.$line(`const ${comp_name} sin_theta = sin(theta);`)
  //     b.$line(`const ${comp_name} a = sin((1 - t) * theta) / sin_theta;`)
  //     b.$line(`const ${comp_name} b = sin(t * theta) / sin_theta;`)
  //     b.$line(`return a * v0 + b * v1;`)
  //   })
  //   b.$line(`}`)
  // }

  // nearly equal
  @math_func("nearly_equal", { accept_comp_kind: ["floating"] })
  static gen_nearly_equal(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;
    const ret_vec_name = dim === 1 ? "bool" : `bool${dim}`;

    if (dim === 1) {
      const init_list = `abs(x - y) <= epsilon`;
      b.$line(`inline ${ret_vec_name} nearly_equal(${vec_name} x, ${vec_name} y, ${comp_name} epsilon = ${comp_name}(0.000001)) { return ${init_list}; }`)
    } else {
      const init_list = _comp_lut
        .slice(0, dim)
        .map(c => `(abs(x.${c} - y.${c}) <= epsilon)`)
        .join(', ');
      b.$line(`inline ${ret_vec_name} nearly_equal(const ${vec_name}& x, const ${vec_name}& y, ${comp_name} epsilon = ${comp_name}(0.000001)) { return { ${init_list} }; }`)
    }
  }

  // clamp radians
  @math_func("clamp_radians", { accept_comp_kind: ["floating"] })
  static gen_clamp_radians(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

    if (dim === 1) {
      b.$line(`inline ${comp_name} clamp_radians(${comp_name} v) {`)
      b.$indent(_b => {
        b.$line(`v = fmod(v, ${comp_name}(kPi2));`)
        b.$line(`if (v < ${comp_name}(0)) {`)
        b.$indent(_b => {
          b.$line(`v += ${comp_name}(kPi2);`)
        })
        b.$line(`}`)
        b.$line(`return v;`)
      })
      b.$line(`}`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `clamp_radians(v.${c})`)
        .join(', ');
      b.$line(`inline ${vec_name} clamp_radians(const ${vec_name}& v) { return { ${init_expr} }; }`)
    }
  }

  // clamp degrees
  @math_func("clamp_degrees", { accept_comp_kind: ["floating"] })
  static gen_clamp_degrees(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

    if (dim === 1) {
      b.$line(`inline ${comp_name} clamp_degrees(${comp_name} v) {`)
      b.$indent(_b => {
        b.$line(`v = fmod(v, ${comp_name}(360));`)
        b.$line(`if (v < ${comp_name}(0)) {`)
        b.$indent(_b => {
          b.$line(`v += ${comp_name}(360);`)
        })
        b.$line(`}`)
        b.$line(`return v;`)
      })
      b.$line(`}`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `clamp_degrees(v.${c})`)
        .join(', ');
      b.$line(`inline ${vec_name} clamp_degrees(const ${vec_name}& v) { return { ${init_expr} }; }`)
    }
  }

  // normalize radians
  @math_func("normalize_radians", { accept_comp_kind: ["floating"] })
  static gen_normalize_radians(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

    if (dim === 1) {
      b.$line(`inline ${comp_name} normalize_radians(${comp_name} v) {`)
      b.$indent(_b => {
        b.$line(`v = clamp_radians(v);`)
        b.$line(`if (v > ${comp_name}(kPi)) {`)
        b.$indent(_b => {
          b.$line(`v -= ${comp_name}(kPi2);`)
        })
        b.$line(`}`)
        b.$line(`return v;`)
      })
      b.$line(`}`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `normalize_radians(v.${c})`)
        .join(', ');
      b.$line(`inline ${vec_name} normalize_radians(const ${vec_name}& v) { return { ${init_expr} }; }`)
    }
  }

  // normalize degrees
  @math_func("normalize_degrees", { accept_comp_kind: ["floating"] })
  static gen_normalize_degrees(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = dim === 1 ? comp_name : `${base_name}${dim}`;

    if (dim === 1) {
      b.$line(`inline ${comp_name} normalize_degrees(${comp_name} v) {`)
      b.$indent(_b => {
        b.$line(`v = clamp_degrees(v);`)
        b.$line(`if (v > ${comp_name}(180)) {`)
        b.$indent(_b => {
          b.$line(`v -= ${comp_name}(360);`)
        })
        b.$line(`}`)
        b.$line(`return v;`)
      })
      b.$line(`}`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `normalize_degrees(v.${c})`)
        .join(', ');
      b.$line(`inline ${vec_name} normalize_degrees(const ${vec_name}& v) { return { ${init_expr} }; }`)
    }
  }

  // square & cube
  @math_func("square", { accept_comp_kind: ["floating"] })
  static gen_square(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v * v`;
      b.$line(`inline ${comp_name} square(${comp_name} v) { return ${init_expr}; }`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `v.${c} * v.${c}`)
        .join(', ');
      b.$line(`inline ${vec_name} square(const ${vec_name}& v) { return { ${init_expr} }; }`)
    }
  }
  @math_func("cube", { accept_comp_kind: ["floating"] })
  static gen_cube(b: CodeBuilder, opt: MathGenOptions) {
    const base_name = opt.base_name;
    const dim = opt.dim;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    if (dim === 1) {
      const init_expr = `v * v * v`;
      b.$line(`inline ${comp_name} cube(${comp_name} v) { return ${init_expr}; }`)
    } else {
      const init_expr = _comp_lut
        .slice(0, dim)
        .map(c => `v.${c} * v.${c} * v.${c}`)
        .join(', ');
      b.$line(`inline ${vec_name} cube(const ${vec_name}& v) { return { ${init_expr} }; }`)
    }
  }
}

export function gen(global_builders: GlobalBuilders, gen_dir: string) {
  const inc_builder = new CodeBuilder()
  inc_builder.$util_header();

  for (const base_name in type_options) {
    const type_opt = type_options[base_name]!;

    const builder = new CodeBuilder()

    // gen header
    builder.$util_header();
    builder.$line(`#include "../vec/bool_vec.hpp"`);
    builder.$line(`#include "../vec/${base_name}_vec.hpp"`);
    builder.$line(`#include "../../math_constants.hpp"`);
    builder.$line(``);
    builder.$line(`namespace skr {`);
    builder.$line(`inline namespace math {`)

    // gen math functions
    const gen_config_dict = (_MathFuncGenerator as any)[_math_gen_config_symbol] as Dict<_MathGenFuncConfig>;
    for (const gen_config_key in gen_config_dict) {
      // get gen config
      const gen_config = gen_config_dict[gen_config_key]!

      // filter by comp kind
      if (!gen_config.accept_comp_kind.includes(type_opt.component_kind)) continue;

      builder.$line(`// ${gen_config.name}`)

      // gen vector
      for (let dim = 1; dim <= 4; ++dim) {
        // filter by dim
        if (!gen_config.accept_dim.includes(dim)) continue;

        gen_config.func.call(_MathFuncGenerator, builder, {
          base_name: base_name,
          dim: dim,
          ...type_opt,
        })
      }

      builder.$line(``)
    }

    builder.$line(`}`)
    builder.$line(`}`)

    // write to file
    const file_name = path.join(gen_dir, `${base_name}_math.hpp`);
    builder.write_file(file_name);
    inc_builder.$line(`#include "./${base_name}_math.hpp"`);
  }

  // write inc file
  const inc_file_name = path.join(gen_dir, `gen_math_func.hpp`);
  inc_builder.write_file(inc_file_name);
}