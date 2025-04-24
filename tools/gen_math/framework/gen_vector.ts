import { CodeBuilder } from "./util"

const _comp_lut = ['x', 'y', 'z', 'w'];
const _arithmetic_ops = ['+', '-', '*', '/', '%']

export enum EVectorCompKind {
  integer,
  float,
  boolean,
}

export interface GenVectorOption {
  fwd_builder: CodeBuilder;
  builder: CodeBuilder;
  base_name: string    // [basename][2/3/4]
  component_name: string // component name
  component_kind: EVectorCompKind; // component kind
};

// generate ctor
function _gen_ctor(dim: number, opt: GenVectorOption) {
  const b = opt.builder;
  const base_name = opt.base_name;
  const comp_name = opt.component_name;
  const vec_name = `${base_name}${dim}`;

  // default ctor
  const default_init = _comp_lut
    .slice(0, dim)
    .map(comp => `${comp}(0)`)
    .join(", ");
  b.$line(`inline ${vec_name}(): ${default_init} {}`);

  // scalar ctor
  const scalar_init = _comp_lut
    .slice(0, dim)
    .map(comp => `${comp}(v)`)
    .join(", ");
  b.$line(`inline ${vec_name}(${comp_name} v): ${scalar_init} {}`);

  // swizzle ctors
  _gen_ctor_swizzle_recursive(
    [],
    dim,
    dim,
    opt
  )
}
function _gen_ctor_swizzle_recursive(
  param_dims: number[],
  remain_dim: number,
  dim: number,
  opt: GenVectorOption
) {
  if (remain_dim !== 0) {
    // do recursive
    for (let i = 1; i <= remain_dim; ++i) {
      if (i == dim) continue; // skip copy ctor
      param_dims.push(i)
      _gen_ctor_swizzle_recursive(
        param_dims,
        remain_dim - i,
        dim,
        opt
      )
      param_dims.pop()
    }
  } else {
    const b = opt.builder;
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const vec_name = `${base_name}${dim}`;

    // generate params
    const params = param_dims
      .map((len, i) => {
        return len == 1 ?
          `${comp_name} v${i}` :
          `${base_name}${len} v${i}`
      })
      .join(", ");

    // generate init list
    let cur_comp = 0;
    const init_list = param_dims
      .flatMap((len, i) => {
        const result = []
        for (let j = 0; j < len; ++j) {
          const visitor = len != 1 ? `.${_comp_lut[j]}` : ``;
          result.push(`${_comp_lut[cur_comp]}(v${i}${visitor})`)
          ++cur_comp
        }
        return result
      })
      .join(", ");

    // dump ctor
    b.$line(`inline ${vec_name}(${params}): ${init_list} {}`);
  }
}

// generate swizzle
function _gen_swizzle(dim: number, opt: GenVectorOption) {
  for (let out_dim = 2; out_dim <= dim; ++out_dim) {
    _gen_swizzle_recursive(
      [],
      out_dim,
      out_dim,
      dim,
      opt
    )
  }
}
function _gen_swizzle_recursive(
  comps: number[],
  remain_dim: number,
  out_dim: number,
  usable_dim: number,
  opt: GenVectorOption
) {
  if (remain_dim !== 0) {
    // do recursive
    for (let i = 0; i < usable_dim; ++i) {
      comps.push(i)
      _gen_swizzle_recursive(
        comps,
        remain_dim - 1,
        out_dim,
        usable_dim,
        opt
      )
      comps.pop()
    }
  } else {
    const b = opt.builder;
    const base_name = opt.base_name;
    const comp_name = opt.component_name;
    const out_vec_name = `${base_name}${out_dim}`;

    // gen getter
    const get_swizzle_name = comps
      .map(comp => _comp_lut[comp])
      .join("");
    const get_init_list = comps
      .map(comp => _comp_lut[comp])
      .join(", ");

    b.$line(`inline ${out_vec_name} ${get_swizzle_name}() const { return {${get_init_list}}; }`);

    {
      let is_unique = true;
      const seen_comp: number[] = [];
      for (const comp of comps) {
        if (seen_comp.find(c => c === comp) !== undefined) {
          is_unique = false;
          break;
        } else {
          seen_comp.push(comp);
        }
      }

      if (is_unique) {
        const set_swizzle_name = "set_" + comps
          .map(comp => _comp_lut[comp])
          .join("");
        const set_assign_list = comps
          .map((comp, i) => `${_comp_lut[comp]} = v.${_comp_lut[i]};`)
          .join(" ");

        b.$line(`inline void ${set_swizzle_name}(const ${out_vec_name}& v) { ${set_assign_list} }`);
      }
    }
  }
}

export function gen(opt: GenVectorOption) {
  const fwd_b = opt.fwd_builder;
  const b = opt.builder;
  const base_name = opt.base_name;
  const comp_name = opt.component_name;
  const comp_kind = opt.component_kind;

  // generate forward declaration
  fwd_b.$line(`// ${base_name} vector, component: ${comp_name}`);
  for (let dim = 2; dim <= 4; ++dim) {
    fwd_b.$line(`struct ${base_name}${dim};`);
  }
  fwd_b.$line(``);

  // generate vector
  for (let dim = 2; dim <= 4; ++dim) {
    const vec_name = `${base_name}${dim}`;

    b.$line(`struct ${vec_name} {`);
    b.$indent(b => {
      // fill components
      b.$line(`${comp_name} ${_comp_lut.slice(0, dim).join(", ")};`)
      b.$line(``)

      // ctor
      b.$line(`// ctor & dtor`);
      _gen_ctor(dim, opt);
      b.$line(`inline ~${vec_name}() = default;`);
      b.$line(``)

      // copy & move & assign & move assign
      b.$line(`// copy & move & assign & move assign`);
      b.$line(`inline ${vec_name}(const ${vec_name}&) = default;`);
      b.$line(`inline ${vec_name}(${vec_name}&&) = default;`);
      b.$line(`inline ${vec_name}& operator=(const ${vec_name}&) = default;`);
      b.$line(`inline ${vec_name}& operator=(${vec_name}&&) = default;`);
      b.$line(``)

      // array assessor
      b.$line(`// array assessor`);
      b.$line(`inline ${comp_name}& operator[](size_t i) {`);
      b.$indent(b => {
        b.$line(`SKR_ASSERT(i >= 0 && i < ${dim} && "index out of range");`);
        b.$line(`return reinterpret_cast<${comp_name}*>(this)[i];`);
      })
      b.$line(`}`);
      b.$line(`inline ${comp_name} operator[](size_t i) const {`);
      b.$indent(b => {
        b.$line(`return const_cast<${vec_name}*>(this)->operator[](i);`);
      })
      b.$line(`}`);
      b.$line(``);

      // unary operator
      if (comp_kind == EVectorCompKind.float ||
        comp_kind == EVectorCompKind.integer) {
        b.$line(`// unary operator`);

        // neg
        const init_list = _comp_lut
          .slice(0, dim)
          .map(comp => `-${comp}`)
          .join(", ");
        b.$line(`inline ${vec_name} operator-() const { return { ${init_list} }; }`);

        b.$line(``);
      } else {
        b.$line(`// unary operator`);

        // not
        const init_list = _comp_lut
          .slice(0, dim)
          .map(comp => `!${comp}`)
          .join(", ");
        b.$line(`inline ${vec_name} operator!() const { return { ${init_list} }; }`);

        b.$line(``);
      }

      // arithmetic operator
      if (comp_kind == EVectorCompKind.float ||
        comp_kind == EVectorCompKind.integer) {
        b.$line(`// arithmetic operator`);
        for (const op of _arithmetic_ops) {
          let init_list, scale_init_list
          if (op === '%' && comp_kind == EVectorCompKind.float) {
            const fmod_name = comp_name === "double" ? "fmod" : "fmodf";
            init_list = _comp_lut
              .slice(0, dim)
              .map(comp => `::std::${fmod_name}(${comp}, rhs.${comp})`)
              .join(", ");
          }
          else {
            init_list = _comp_lut
              .slice(0, dim)
              .map(comp => `${comp} ${op} rhs.${comp}`)
              .join(", ");
          }
          b.$line(`inline ${vec_name} operator${op}(const ${vec_name}& rhs) const { return { ${init_list} }; }`);
        }
        b.$line(``);

      }


      // swizzle
      b.$line(`// swizzle`);
      _gen_swizzle(dim, opt);
      b.$line(``);

      // hash
      b.$line(`// hash`);
      b.$line(`inline static size_t _skr_hash(const ${vec_name}& v) {`);
      b.$indent(b => {
        b.$line(`auto hasher = ::skr::Hash<${comp_name}>{};`)
        b.$line(`auto result = hasher(v.${_comp_lut[0]});`);
        for (let i = 1; i < dim; ++i) {
          b.$line(`result = ::skr::hash_combine(result, hasher(v.${_comp_lut[i]}));`);
        }
        b.$line(`return result;`);
      })
      b.$line(`}`)
    })
    b.$line(`};`);
  }
}