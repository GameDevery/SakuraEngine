using System.Text;

namespace SB.Cli;

#region Build output

public static class CliStyleStrings
{
    public const string Clear = "\u001b[0m";

    // style
    public const string Bold = "\u001b[1m";
    public const string NoBold = "\u001b[22m";
    public const string Underline = "\u001b[4m";
    public const string NoUnderline = "\u001b[24m";
    public const string Reverse = "\u001b[7m";
    public const string NoReverse = "\u001b[27m";

    // front colors
    public const string FrontGray = "\u001b[30m";
    public const string FrontRed = "\u001b[31m";
    public const string FrontGreen = "\u001b[32m";
    public const string FrontYellow = "\u001b[33m";
    public const string FrontBlue = "\u001b[34m";
    public const string FrontMagenta = "\u001b[35m";
    public const string FrontCyan = "\u001b[36m";
    public const string FrontWhite = "\u001b[37m";

    // back colors
    public const string BackGray = "\u001b[40m";
    public const string BackRed = "\u001b[41m";
    public const string BackGreen = "\u001b[42m";
    public const string BackYellow = "\u001b[43m";
    public const string BackBlue = "\u001b[44m";
    public const string BackMagenta = "\u001b[45m";
    public const string BackCyan = "\u001b[46m";
    public const string BackWhite = "\u001b[47m";
}
public class CliOutputBuilder
{
    #region Style
    public CliOutputBuilder StyleClear()
    {
        _Content.Append(CliStyleStrings.Clear);
        return this;
    }

    // style
    public CliOutputBuilder StyleBold()
    {
        _Content.Append(CliStyleStrings.Bold);
        return this;
    }
    public CliOutputBuilder StyleNoBold()
    {
        _Content.Append(CliStyleStrings.NoBold);
        return this;
    }
    public CliOutputBuilder StyleUnderline()
    {
        _Content.Append(CliStyleStrings.Underline);
        return this;
    }
    public CliOutputBuilder StyleNoUnderline()
    {
        _Content.Append(CliStyleStrings.NoUnderline);
        return this;
    }
    public CliOutputBuilder StyleReverse()
    {
        _Content.Append(CliStyleStrings.Reverse);
        return this;
    }
    public CliOutputBuilder StyleNoReverse()
    {
        _Content.Append(CliStyleStrings.NoReverse);
        return this;
    }

    // front colors
    public CliOutputBuilder StyleFrontGray()
    {
        _Content.Append(CliStyleStrings.FrontGray);
        return this;
    }
    public CliOutputBuilder StyleFrontRed()
    {
        _Content.Append(CliStyleStrings.FrontRed);
        return this;
    }
    public CliOutputBuilder StyleFrontGreen()
    {
        _Content.Append(CliStyleStrings.FrontGreen);
        return this;
    }
    public CliOutputBuilder StyleFrontYellow()
    {
        _Content.Append(CliStyleStrings.FrontYellow);
        return this;
    }
    public CliOutputBuilder StyleFrontBlue()
    {
        _Content.Append(CliStyleStrings.FrontBlue);
        return this;
    }
    public CliOutputBuilder StyleFrontMagenta()
    {
        _Content.Append(CliStyleStrings.FrontMagenta);
        return this;
    }
    public CliOutputBuilder StyleFrontCyan()
    {
        _Content.Append(CliStyleStrings.FrontCyan);
        return this;
    }
    public CliOutputBuilder StyleFrontWhite()
    {
        _Content.Append(CliStyleStrings.FrontWhite);
        return this;
    }

    // back colors
    public CliOutputBuilder StyleBackGray()
    {
        _Content.Append(CliStyleStrings.BackGray);
        return this;
    }
    public CliOutputBuilder StyleBackRed()
    {
        _Content.Append(CliStyleStrings.BackRed);
        return this;
    }
    public CliOutputBuilder StyleBackGreen()
    {
        _Content.Append(CliStyleStrings.BackGreen);
        return this;
    }
    public CliOutputBuilder StyleBackYellow()
    {
        _Content.Append(CliStyleStrings.BackYellow);
        return this;
    }
    public CliOutputBuilder StyleBackBlue()
    {
        _Content.Append(CliStyleStrings.BackBlue);
        return this;
    }
    public CliOutputBuilder StyleBackMagenta()
    {
        _Content.Append(CliStyleStrings.BackMagenta);
        return this;
    }
    public CliOutputBuilder StyleBackCyan()
    {
        _Content.Append(CliStyleStrings.BackCyan);
        return this;
    }
    public CliOutputBuilder StyleBackWhite()
    {
        _Content.Append(CliStyleStrings.BackWhite);
        return this;
    }
    #endregion

    #region Build Content
    public CliOutputBuilder Write(string content)
    {
        _Content.Append(content);
        _SolveIndent(content, ref _IndentCache);
        return this;
    }
    public CliOutputBuilder WriteLine(string content)
    {
        _Content.AppendLine(content);
        _IndentCache = 0;
        return this;
    }
    public CliOutputBuilder NextLine()
    {
        _Content.AppendLine();
        _IndentCache = 0;
        return this;
    }
    public CliOutputBuilder WriteKeepIndent(string content)
    {
        // solve indent
        uint newIndent = 0;
        _SolveIndent(content, ref newIndent);

        // apply indent
        if (_IndentCache > 0)
        {
            var indentStr = "\n" + new string(' ', (int)_IndentCache);
            content = content.Replace("\n", indentStr);
        }

        // update state
        _Content.Append(content);
        _IndentCache += newIndent;
        return this;
    }
    #endregion

    #region Helpers
    private void _SolveIndent(string str, ref uint indent)
    {
        var lastNewLine = str.LastIndexOf('\n');
        if (lastNewLine >= 0)
        {
            indent = (uint)(str.Length - lastNewLine - 1);
        }
        else
        {
            indent += (uint)str.Length;
        }
    }
    #endregion


    private StringBuilder _Content = new StringBuilder();
    private uint _IndentCache = 0;
};
#endregion

#region Parse command line
public class CmdToken
{
    public enum EKind
    {
        Argument, // any token not starting with '-'
        Option, // --xxx, --xxx=yyy
        ShortOption, // -a, -a=xxx, -abc(means -a -b -c), -abc=false
        DoubleDash, // --, means end of parse, and push all command into rest
        Unparsed,// any token after --
    };

    #region getters
    public EKind Kind => _Kind;
    public string Raw => _Raw;
    public bool IsArgument => _Kind == EKind.Argument;
    public bool IsOption => _Kind == EKind.Option;
    public bool IsShortOption => _Kind == EKind.ShortOption;
    public bool IsDoubleDash => _Kind == EKind.DoubleDash;
    public bool IsUnparsed => _Kind == EKind.Unparsed;
    public string Argument
    {
        get
        {
            if (_Kind != EKind.Argument) throw new InvalidOperationException("Not an Argument token");
            return _Raw;
        }
    }
    public string OptionName
    {
        get
        {
            if (_Kind != EKind.Option) throw new InvalidOperationException("Not an Option token");

            var eqIdx = _Raw.IndexOf('=');
            if (eqIdx >= 0)
            {
                return _Raw.Substring(2, eqIdx - 2);
            }
            else
            {
                return _Raw.Substring(2);
            }
        }
    }
    public string? OptionValue
    {
        get
        {
            if (_Kind != EKind.Option) throw new InvalidOperationException("Not an Option token");

            var eqIdx = _Raw.IndexOf('=');
            if (eqIdx >= 0)
            {
                return _Raw.Substring(eqIdx + 1);
            }
            else
            {
                return null;
            }
        }
    }
    public List<string> ShortOptionNames
    {
        get
        {
            if (_Kind != EKind.ShortOption) throw new InvalidOperationException("Not a ShortOption token");

            var eqIdx = _Raw.IndexOf('=');
            var namesPart = eqIdx >= 0 ? _Raw.Substring(1, eqIdx - 1) : _Raw.Substring(1);
            var names = new List<string>();
            foreach (var c in namesPart)
            {
                names.Add(c.ToString());
            }
            return names;
        }
    }
    public string? ShortOptionValue
    {
        get
        {
            if (_Kind != EKind.ShortOption) throw new InvalidOperationException("Not a ShortOption token");

            var eqIdx = _Raw.IndexOf('=');
            if (eqIdx >= 0)
            {
                return _Raw.Substring(eqIdx + 1);
            }
            else
            {
                return null;
            }
        }
    }
    #endregion


    private EKind _Kind;
    private string _Raw = "";
};
#endregion

#region Attributes
/// <summary>
/// Attribute to mark a property as a command-line option
/// </summary>
[AttributeUsage(AttributeTargets.Property)]
public class OptionAttribute : Attribute
{
    public string Name { get; set; } = string.Empty;
    public char? ShortName { get; set; } = null;
    public string Help { get; set; } = string.Empty;
    public bool IsRequired { get; set; } = false;
    public List<string>? Selections { get; set; } = null;
}

/// <summary>
/// Attribute to mark a property as a sub-command
/// </summary>
[AttributeUsage(AttributeTargets.Property)]
public class SubCmdAttribute : Attribute
{
    public string Name { get; set; } = string.Empty;
    public char? ShortName { get; set; } = null;
    public string Help { get; set; } = string.Empty;
    public string Usage { get; set; } = string.Empty;
}

/// <summary>
/// Attribute to mark a property as the rest options, which collects all unparsed arguments
/// </summary>
[AttributeUsage(AttributeTargets.Property)]
public class RestOptionsAttribute : Attribute
{
    public bool AllowUnparsed { get; set; } = false;
    public bool RequireDoubleDash { get; set; } = false;
}

/// <summary>
/// Attribute to mark a method as the command execution handler
/// </summary>
[AttributeUsage(AttributeTargets.Method)]
public class ExecCmdAttribute : Attribute
{
}

/// <summary>
/// Attribute to mark a class to register commands to the default parser
/// </summary>
/// <returns></returns>
public delegate object RegisterCmdDelegate();
[AttributeUsage(AttributeTargets.Class)]
public class RegisterCmdAttribute : Attribute
{
    public RegisterCmdDelegate? RegisterDelegate { get; set; } = null;
}
#endregion

#region cmd parser
public class CommandParser
{
}
#endregion