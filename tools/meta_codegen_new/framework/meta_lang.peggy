start
  = AssignExpr

_ "whitespace"
  = [ \t\n\r]*

Name
  = $([a-zA-Z0-9_]+)

Number
  = [-]?[0-9]+.[0-9]+ { return parseFloat(text(), 10) }
  / [-]?[0-9]+ { return parseInt(text(), 10) }

String
  = "\'"content:$[^\']*"\'" { return content; }

Boolean
  = "true" { return true }
  / "false" { return false }

ValueExpr
  = value:(Number / String / Boolean) {
    return {
      type: "value",
      value: value,
    }
  }

Preset
  = "@" content:Name { return content }

PresetCombineExpr
  = head:Preset tail:(_ "+" _ Preset)* {
    const presets = tail.reduce(
      (result, elem) => {
        result.push(elem[3]); 
        return result;
      }, [head]
    );
    return {
      type: "presets",
      value: presets,
    }
  }

AccessExpr
  = head:Name tail:("." Name)* { return tail.reduce(
    (result, elem)=>{
      result.push(elem[1]); 
      return result;
    }, [head]); 
  }

AssignExpr
  = left:AccessExpr _ op:("="/"+=") _ right:(ValueExpr/PresetCombineExpr) { 
    return {
      visitor: left,
      op: op,
      value: right,
    }
  }