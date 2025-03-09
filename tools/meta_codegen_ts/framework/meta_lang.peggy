{{
function Literal(location, value) {
  return {
    type: 'literal',
    value: value,
    location: location,
  };
}
function Identifier(location, value) {
  return {
    type: 'identifier',
    value: value,
    location: location,
  };
}
function AccessExpr(location, value) {
  return {
    type: 'access',
    value: value,
    location: location,
  };
}
function Operator(location, op, left, right) {
  return {
    type: 'operator',
    op: op,
    left: left,
    right: right,
    location: location,
  };
}
function Preset(location, value) {
  return {
    type: 'preset',
    value: value,
    location: location,
  };
}
function PresetExpr(location, value) {
  return {
    type: 'preset_expr',
    value: value,
    location: location,
  };
}
function ArrayExpr(location, value) {
  return {
    type: 'array_expr',
    value: value,
    location: location,
  };
}
function processStringLiteral(str) {
  return str.slice(1, -1)
}
}}

start
  = AssignExprs

_ "whitespace"
  = [ \t\n\r]*

Identifier "Identifier"
  = $([a-zA-Z0-9_]+) { return Identifier(location(), text()) }

Number "number"
  = [-]?[0-9]+.[0-9]+ { return Literal(location(), parseFloat(text(), 10)) }
  / [-]?[0-9]+ { return Literal(location(), parseInt(text(), 10)) }

Boolean "boolean(true/false)"
  = "true" { return Literal(location(), true) }
  / "false" { return Literal(location(), false) }

String "string"
  = "\"" StringContentDoubleQuote* "\"" { return Literal(location(), processStringLiteral(text())) }
  / "'" StringContentQuote* "'" { return Literal(location(), processStringLiteral(text())) }
  / "`" StringContentBackQuote* "`" { return Literal(location(), processStringLiteral(text())) }

StringContentDoubleQuote
  = [^\r\n"\\]+
  / EscapeSequence

StringContentQuote
  = [^\r\n'\\]+
  / EscapeSequence

StringContentBackQuote
  = [^\r\n`\\]+
  / EscapeSequence

EscapeSequence
  = "\\" ([A-Za-z"'`\\] / [0-9]+)

ValueExpr "values"
  = (Number / String / Boolean)

ArrayExpr "array([1,2,\"hello\"])"
  = "[" _ content:ValueExpr|..,_","_| _ "]" { return ArrayExpr(location(), content) }

Preset "preset(@xxx)"
  = &"@" "@"content:Identifier { return Preset(location(), content.value) }

PresetExpr "presets(@xxx|@xxx|@xxx)"
  = content:Preset|1..,_ "|" _| { return PresetExpr(location(), content) }

AccessExpr "access(a.b.c)"
  = content:Identifier|..,"."| { return AccessExpr(location(), content) }

AssignExpr
  = visitor:AccessExpr _ op:("="/"+=") _ value:(ArrayExpr/ValueExpr/PresetExpr) { 
      return Operator(location(), op, visitor, value);
  }

AssignExprs
  = _ head:AssignExpr _ ";"? tails:(_ AssignExpr _ ";"?)* _ { return [head].concat(tails.map(t => t[1])) }