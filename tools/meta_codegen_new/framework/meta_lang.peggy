start
  = AssignExprs

_ "whitespace"
  = [ \t\n\r]*

Name "name(alpha/number/_)"
  = $([a-zA-Z0-9_]+)

Number "number"
  = [-]?[0-9]+.[0-9]+ { return parseFloat(text(), 10) }
  / [-]?[0-9]+ { return parseInt(text(), 10) }

String "string('xxx')"
  = "\`"content:$[^\`]*"\`" { return content; }

Boolean "boolean(true/false)"
  = "true" { return true }
  / "false" { return false }

ValueExpr "values"
  = value:(Number / String / Boolean) { return { type: "value", value: value, } }

ArrayExpr "array([1,2,\"hello\"])"
  = "[" _ content:ValueExpr|..,_","_| _ "]" { return { type:"array", value: content } }

Preset "preset(@xxx)"
  = "@" content:Name { return content }

PresetExpr "presets(@xxx|@xxx|@xxx)"
  = content:Preset|..,_ "|" _| { return { type: "preset", value: content } }

AccessExpr "access(a.b.c)"
  = Name|..,"."|

AssignExpr
  = visitor:AccessExpr _ op:("="/"+=") _ value:(ArrayExpr/ValueExpr/PresetExpr) { 
    return {
      visitor: visitor,
      op: op,
      value: value,
    }
  }

AssignExprs
  = _ head:AssignExpr _ ";"? tails:(_ AssignExpr _ ";"?)* { return [head].concat(tails.map(t => t[1])) }