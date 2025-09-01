using System.Reflection;
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

    #region Output
    public void Dump()
    {
        Console.Write(_Content.ToString());
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


    public void Parse(string raw)
    {
        _Raw = raw;

        if (_Raw == "--")
        {
            _Kind = EKind.DoubleDash;
        }
        else if (_Raw.StartsWith("--"))
        {
            _Kind = EKind.Option;
        }
        else if (_Raw.StartsWith("-") && _Raw.Length >= 2)
        {
            _Kind = EKind.ShortOption;
        }
        else
        {
            _Kind = EKind.Argument;
        }
    }
    public void Unparsed(string raw)
    {
        _Raw = raw;
        _Kind = EKind.Unparsed;
    }

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
public delegate void CommandExec();
public class CommandParser
{
    // register command


    // invoke command line
    public bool Invoke(string[] args)
    {
        // parse args
        List<CmdToken> tokens = new();
        {
            bool doNotParse = false;
            for (int i = 1; i < args.Length; ++i)
            {
                if (doNotParse)
                {
                    var token = new CmdToken();
                    token.Unparsed(args[i]);
                    tokens.Add(token);
                }
                else
                {
                    var token = new CmdToken();
                    token.Parse(args[i]);
                    if (token.IsDoubleDash)
                    {
                        doNotParse = true;
                    }
                    tokens.Add(token);
                }
            }
        }

        // solve sub command


        return true;
    }


    private class OptionData
    {
        public required PropertyInfo Property;
        public required object? Instance;
        public required OptionAttribute Attribute;

        public bool IsBool => Property.PropertyType == typeof(bool) || Property.PropertyType == typeof(bool?);
        public bool IsInt => Property.PropertyType == typeof(int) || Property.PropertyType == typeof(int?);
        public bool isUint => Property.PropertyType == typeof(uint) || Property.PropertyType == typeof(uint?);
        public bool IsFloat => Property.PropertyType == typeof(float) || Property.PropertyType == typeof(float?) || Property.PropertyType == typeof(double) || Property.PropertyType == typeof(double?);
        public bool IsDouble => Property.PropertyType == typeof(double) || Property.PropertyType == typeof(double?);
        public bool IsString => Property.PropertyType == typeof(string);
        public bool IsStringList => Property.PropertyType == typeof(List<string>);

        public string DumpTypeName()
        {
            var type = Property.PropertyType;
            if (IsBool)
            {
                return "<bool>";
            }
            else if (IsInt)
            {
                return "<int>";
            }
            else if (isUint)
            {
                return "<uint>";
            }
            else if (IsFloat || IsDouble)
            {
                return "<float>";
            }
            else if (IsString)
            {
                return "<string>";
            }
            else if (IsStringList)
            {
                return "<string...>";
            }
            else
            {
                return $"<unknown>";
            }
        }
    }
    private class Node
    {
        public CommandExec? exec;
        public SubCmdAttribute Attribute;
        public List<OptionData> Options = new();
        public List<Node> SubCommands = new();

        public Node(SubCmdAttribute attr)
        {
            Attribute = attr;
        }
        public Node(object instance, SubCmdAttribute attr)
        {
            Attribute = attr;
            var type = instance.GetType();

            // solve options and sub commands
            foreach (var prop in type.GetProperties())
            {
                // add options
                var optionAttr = prop.GetCustomAttribute<OptionAttribute>();
                if (optionAttr != null)
                {
                    Options.Add(new OptionData()
                    {
                        Property = prop,
                        Instance = instance,
                        Attribute = optionAttr
                    });
                    continue;
                }

                // add sub command
                var subCmdAttr = prop.GetCustomAttribute<SubCmdAttribute>();
                if (subCmdAttr != null)
                {
                    var node = new Node(prop.GetValue(instance)!, subCmdAttr);
                    SubCommands.Add(node);
                }
            }

            // find exec method
            foreach (var method in type.GetMethods())
            {
                var execAttr = method.GetCustomAttribute<ExecCmdAttribute>();
                if (execAttr != null)
                {
                    // check signature
                    if (method.GetParameters().Length != 0 || method.ReturnType != typeof(void))
                    {
                        throw new InvalidOperationException("ExecCmd method must have no parameters and return void");
                    }

                    // create delegate
                    exec = (CommandExec)Delegate.CreateDelegate(typeof(CommandExec), instance, method);
                    return;
                }
            }
        }
        public void CheckOptions()
        {
            Dictionary<string, OptionData> seenOptions = new();
            Dictionary<char, OptionData> seenShortOptions = new();

            foreach (var option in Options)
            {
                // check name
                if (seenOptions.ContainsKey(option.Attribute.Name))
                {
                    throw new InvalidOperationException($"Duplicate option name: {option.Attribute.Name}");
                }
                seenOptions[option.Attribute.Name] = option;

                // check short name
                if (option.Attribute.ShortName != null)
                {
                    if (seenShortOptions.ContainsKey(option.Attribute.ShortName.Value))
                    {
                        throw new InvalidOperationException($"Duplicate short option name: {option.Attribute.ShortName}");
                    }
                    seenShortOptions[option.Attribute.ShortName.Value] = option;
                }
            }
        }
        public void CheckSubCommands()
        {
            Dictionary<string, Node> seenSubCommands = new();
            Dictionary<char, Node> seenShortSubCommands = new();

            foreach (var subCmd in SubCommands)
            {
                // check name
                if (seenSubCommands.ContainsKey(subCmd.Attribute.Name))
                {
                    throw new InvalidOperationException($"Duplicate sub command name: {subCmd.Attribute.Name}");
                }
                seenSubCommands[subCmd.Attribute.Name] = subCmd;

                // check short name
                if (subCmd.Attribute.ShortName != null)
                {
                    if (seenShortSubCommands.ContainsKey(subCmd.Attribute.ShortName.Value))
                    {
                        throw new InvalidOperationException($"Duplicate short sub command name: {subCmd.Attribute.ShortName}");
                    }
                    seenShortSubCommands[subCmd.Attribute.ShortName.Value] = subCmd;
                }
            }
        }

        // help
        public void PrintHelp()
        {
            var builder = new CliOutputBuilder();

            // print sakura build logo
            builder
                .StyleBold().StyleFrontGreen()
                .WriteLine(@"")
                .WriteLine(@"")
                .WriteLine(@"    _____         _                        ____          _  _      _ ")
                .WriteLine(@"   / ____|       | |                      |  _ \        (_)| |    | |")
                .WriteLine(@"  | (___    __ _ | | __ _   _  _ __  __ _ | |_) | _   _  _ | |  __| |")
                .WriteLine(@"   \___ \  / _` || |/ /| | | || '__|/ _` ||  _ < | | | || || | / _` |")
                .WriteLine(@"   ____) || (_| ||   < | |_| || |  | (_| || |_) || |_| || || || (_| |")
                .WriteLine(@"  |_____/  \__,_||_|\_\ \__,_||_|   \__,_||____/  \__,_||_||_| \__,_|")
                .WriteLine(@"")
                .WriteLine(@"")
                .StyleClear();

            // print self usage
            builder
                // usage:
                .StyleBold()
                .Write("Usage: ")
                .StyleClear()
                // content
                .StyleFrontBlue()
                .Write(Attribute.Usage)
                .StyleClear()
                .NextLine();

            // print sub commands
            if (SubCommands.Count != 0)
            {
                builder
                    .StyleBold()
                    .Write("Sub Commands:")
                    .StyleClear()
                    .NextLine();

                // solve max sub command length
                int maxSubCmdLength = 0;
                foreach (var subCmd in SubCommands)
                {
                    int subCmdLen = 0;
                    subCmdLen += 3; // 'X, '
                    subCmdLen += subCmd.Attribute.Name.Length; // 'XXXX'
                    maxSubCmdLength = Math.Max(subCmdLen, maxSubCmdLength);
                }

                // print sub commands
                foreach (var subCmd in SubCommands)
                {
                    // solve sub command str
                    string subCmdStr;
                    if (subCmd.Attribute.ShortName != null)
                    {
                        subCmdStr = $"{subCmd.Attribute.ShortName}, {subCmd.Attribute.Name}";
                    }
                    else
                    {
                        subCmdStr = $"    {subCmd.Attribute.Name}";
                    }
                    subCmdStr = subCmdStr.PadRight(maxSubCmdLength);

                    builder
                        // write sub command
                        .StyleFrontMagenta()
                        .StyleBold()
                        .Write($"  {subCmdStr} : ")
                        .StyleClear()
                        // write help
                        .StyleClear()
                        .WriteKeepIndent(subCmd.Attribute.Help)
                        .NextLine();
                }
            }

            // print options
            if (Options.Count != 0)
            {
                builder
                    .StyleBold()
                    .WriteLine("Options:")
                    .StyleClear();

                // solve max option length
                int maxOptionLength = 0;
                int maxTypeLength = 0;
                foreach (var option in Options)
                {
                    int optionLen = 0;
                    optionLen += 4; // '-X, '
                    optionLen += 2 + option.Attribute.Name.Length; // '--XXXX'
                    maxOptionLength = Math.Max(optionLen, maxOptionLength);

                    int typeLen = option.DumpTypeName().Length;
                    maxTypeLength = Math.Max(typeLen, maxTypeLength);
                }

                // print options
                foreach (var option in Options)
                {
                    // solve type str
                    string typeStr = option.DumpTypeName().PadRight(maxTypeLength);

                    // solve options str
                    string optionStr;
                    if (option.Attribute.ShortName != null)
                    {
                        optionStr = $"-{option.Attribute.ShortName}, --{option.Attribute.Name}";
                    }
                    else
                    {
                        optionStr = $"    --{option.Attribute.Name}";
                    }
                    optionStr = optionStr.PadRight(maxOptionLength);

                    string requiredStr = option.Attribute.IsRequired ? "[REQUIRED]" : "";

                    builder
                        .Write("  ")
                        // write type
                        .StyleFrontYellow()
                        .Write($"  {typeStr} ")
                        .StyleClear()
                        // write option
                        .StyleFrontGreen()
                        .StyleBold()
                        .Write($"{optionStr} : ")
                        .StyleClear()
                        // write help
                        .StyleClear()
                        .WriteKeepIndent($"{requiredStr}{option.Attribute.Help}")
                        .NextLine();
                }
            }

            builder.Dump();
        }
    }
    private Node? rootCommand;
}
#endregion