using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using bottlenoselabs.C2CS.Runtime;
using static Tracy.PInvoke;

public static class Profiler
{
    // Plot names need to be cached for the lifetime of the program
    // seealso Tracy docs section 3.1
    private static readonly Dictionary<string, CString> PlotNameCache = new Dictionary<string, CString>();
    private static readonly bool EnableProfile = false;

    /// <summary>
    /// Begins a new <see cref="ProfilerZone"/> and returns the handle to that zone. Time
    /// spent inside a zone is calculated by Tracy and shown in the profiler. A zone is
    /// ended when <see cref="ProfilerZone.Dispose"/> is called either automatically via 
    /// disposal scope rules or by calling it manually.
    /// </summary>
    /// <param name="zoneName">A custom name for this zone.</param>
    /// <param name="active">Is the zone active. An inactive zone wont be shown in the profiler.</param>
    /// <param name="color">An <c>RRGGBB</c> color code that Tracy will use to color the zone in the profiler.</param>
    /// <param name="text">Arbitrary text associated with this zone.</param>
    /// <param name="lineNumber">
    /// The source code line number that this zone begins at.
    /// If this param is not explicitly assigned the value will provided by <see cref="CallerLineNumberAttribute"/>.
    /// </param>
    /// <param name="filePath">
    /// The source code file path that this zone begins at.
    /// If this param is not explicitly assigned the value will provided by <see cref="CallerFilePathAttribute"/>.
    /// </param>
    /// <param name="memberName">
    /// The source code member name that this zone begins at.
    /// If this param is not explicitly assigned the value will provided by <see cref="CallerMemberNameAttribute"/>.
    /// </param>
    /// <returns></returns>
    public static ProfilerZone BeginZone(
        string? zoneName = null,
        bool active = true,
        uint color = 0,
        string? text = null,
        [CallerLineNumber] uint lineNumber = 0,
        [CallerFilePath] string? filePath = null,
        [CallerMemberName] string? memberName = null)
    {
        if (EnableProfile)
        {
            using var filestr = GetCString(filePath, out var fileln);
            using var memberstr = GetCString(memberName, out var memberln);
            using var namestr = GetCString(zoneName, out var nameln);
            var srcLocId = TracyAllocSrclocName(lineNumber, filestr, fileln, memberstr, memberln, namestr, nameln);
            var context = TracyEmitZoneBeginAlloc(srcLocId, active ? 1 : 0);
            TracyEmitZoneColor(context, color);
            if (text != null)
            {
                using var textstr = GetCString(text, out var textln);
                TracyEmitZoneText(context, textstr, textln);
            }

            return new ProfilerZone(context);
        }
        return default;
    }

    /// <summary>
    /// Configure how Tracy will display plotted values.
    /// </summary>
    /// <param name="name">
    /// Name of the plot to configure. Each <paramref name="name"/> represents a unique plot.
    /// </param>
    /// <param name="type">
    /// Changes how the values in the plot are presented by the profiler.
    /// </param>
    /// <param name="step">
    /// Determines whether the plot will be displayed as a staircase or will smoothly change between plot points
    /// </param>
    /// <param name="fill">
    /// If <see langword="false"/> the the area below the plot will not be filled with a solid color.
    /// </param>
    /// <param name="color">
    /// An <c>RRGGBB</c> color code that Tracy will use to color the plot in the profiler.
    /// </param>
    public static void PlotConfig(string name, PlotType type = PlotType.Number, bool step = false, bool fill = true, uint color = 0)
    {
        if (EnableProfile)
        {
            var namestr = GetPlotCString(name);
            TracyEmitPlotConfig(namestr, (int)type, step ? 1 : 0, fill ? 1 : 0, color);
        }
    }

    /// <summary>
    /// Add a <see langword="double"/> value to a plot.
    /// </summary>
    public static void Plot(string name, double val)
    {
        if (EnableProfile)
        {
            var namestr = GetPlotCString(name);
            TracyEmitPlot(namestr, val);
        }
    }

    /// <summary>
    /// Add a <see langword="float"/> value to a plot.
    /// </summary>
    public static void Plot(string name, int val)
    {
        if (EnableProfile)
        {
            var namestr = GetPlotCString(name);
            TracyEmitPlotInt(namestr, val);
        }
    }

    /// <summary>
    /// Add a <see langword="float"/> value to a plot.
    /// </summary>
    public static void Plot(string name, float val)
    {
        if (EnableProfile)
        {
            var namestr = GetPlotCString(name);
            TracyEmitPlotFloat(namestr, val);
        }
    }

    private static CString GetPlotCString(string name)
    {
        if (!PlotNameCache.TryGetValue(name, out var plotCString))
        {
            plotCString = CString.FromString(name);
            PlotNameCache.Add(name, plotCString);
        }
        return plotCString;
    }

    /// <summary>
    /// Emit a string that will be included along with the trace description.
    /// </summary>
    /// <remarks>
    /// Viewable in the Info tab in the profiler.
    /// </remarks>
    public static void AppInfo(string appInfo)
    {
        using var infostr = GetCString(appInfo, out var infoln);
        TracyEmitMessageAppinfo(infostr, infoln);
    }

    public static void Message(string message)
    {
        if (EnableProfile)
        {
            using var messagestr = GetCString(message, out var messageln);
            TracyEmitMessage(messagestr, messageln, 0);
        }
    }


    /// <summary>
    /// Emit the top-level frame marker.
    /// </summary>
    /// <remarks>
    /// Tracy Cpp API and docs refer to this as the <c>FrameMark</c> macro.
    /// </remarks>
    public static void EmitFrameMark()
    {
        TracyEmitFrameMark(null);
    }

    /// <summary>
    /// Is the app connected to the external profiler?
    /// </summary>
    /// <returns></returns>
    public static bool IsConnected()
    {
        return TracyConnected() != 0;
    }

    /// <summary>
    /// Creates a <seealso cref="CString"/> for use by Tracy. Also returns the
    /// length of the string for interop convenience.
    /// </summary>
    public static CString GetCString(string? fromString, out ulong clength)
    {
        if (fromString == null)
        {
            clength = 0;
            return new CString(0);
        }
        clength = (ulong)fromString.Length;
        return CString.FromString(fromString);
    }

    public enum PlotType
    {
        /// <summary>
        /// Values will be displayed as plain numbers.
        /// </summary>
        Number = 0,

        /// <summary>
        /// Treats the values as memory sizes. Will display kilobytes, megabytes, etc.
        /// </summary>
        Memory = 1,

        /// <summary>
        /// Values will be displayed as percentage (with value 100 being equal to 100%).
        /// </summary>
        Percentage = 2,
    }

    public readonly struct ProfilerZone : IDisposable
    {
        public readonly TracyCZoneCtx Context;

        public uint Id => Context.Data.Id;

        public int Active => Context.Data.Active;

        internal ProfilerZone(TracyCZoneCtx context)
        {
            Context = context;
        }

        public void EmitName(string name)
        {
            if (EnableProfile)
            {
                using var namestr = Profiler.GetCString(name, out var nameln);
                TracyEmitZoneName(Context, namestr, nameln);
            }
        }

        public void EmitColor(uint color)
        {
            if (EnableProfile)
                TracyEmitZoneColor(Context, color);
        }

        public void EmitText(string text)
        {
            if (EnableProfile)
            {
                using var textstr = Profiler.GetCString(text, out var textln);
                TracyEmitZoneText(Context, textstr, textln);
            }
        }

        public void Dispose()
        {
            if (EnableProfile)
                TracyEmitZoneEnd(Context);
        }
    }

    /// <summary>
    /// An enum of predefined colors for use with Tracy.
    /// </summary>
    /// <remarks>
    /// From <c>TracyColor.hpp</c>
    /// </remarks>
    public enum ColorType : uint
    {
        Snow = 0xfffafa,
        GhostWhite = 0xf8f8ff,
        WhiteSmoke = 0xf5f5f5,
        Gainsboro = 0xdcdcdc,
        FloralWhite = 0xfffaf0,
        OldLace = 0xfdf5e6,
        Linen = 0xfaf0e6,
        AntiqueWhite = 0xfaebd7,
        PapayaWhip = 0xffefd5,
        BlanchedAlmond = 0xffebcd,
        Bisque = 0xffe4c4,
        PeachPuff = 0xffdab9,
        NavajoWhite = 0xffdead,
        Moccasin = 0xffe4b5,
        Cornsilk = 0xfff8dc,
        Ivory = 0xfffff0,
        LemonChiffon = 0xfffacd,
        Seashell = 0xfff5ee,
        Honeydew = 0xf0fff0,
        MintCream = 0xf5fffa,
        Azure = 0xf0ffff,
        AliceBlue = 0xf0f8ff,
        Lavender = 0xe6e6fa,
        LavenderBlush = 0xfff0f5,
        MistyRose = 0xffe4e1,
        White = 0xffffff,
        Black = 0x000000,
        DarkSlateGray = 0x2f4f4f,
        DarkSlateGrey = 0x2f4f4f,
        DimGray = 0x696969,
        DimGrey = 0x696969,
        SlateGray = 0x708090,
        SlateGrey = 0x708090,
        LightSlateGray = 0x778899,
        LightSlateGrey = 0x778899,
        Gray = 0xbebebe,
        Grey = 0xbebebe,
        X11Gray = 0xbebebe,
        X11Grey = 0xbebebe,
        WebGray = 0x808080,
        WebGrey = 0x808080,
        LightGrey = 0xd3d3d3,
        LightGray = 0xd3d3d3,
        MidnightBlue = 0x191970,
        Navy = 0x000080,
        NavyBlue = 0x000080,
        CornflowerBlue = 0x6495ed,
        DarkSlateBlue = 0x483d8b,
        SlateBlue = 0x6a5acd,
        MediumSlateBlue = 0x7b68ee,
        LightSlateBlue = 0x8470ff,
        MediumBlue = 0x0000cd,
        RoyalBlue = 0x4169e1,
        Blue = 0x0000ff,
        DodgerBlue = 0x1e90ff,
        DeepSkyBlue = 0x00bfff,
        SkyBlue = 0x87ceeb,
        LightSkyBlue = 0x87cefa,
        SteelBlue = 0x4682b4,
        LightSteelBlue = 0xb0c4de,
        LightBlue = 0xadd8e6,
        PowderBlue = 0xb0e0e6,
        PaleTurquoise = 0xafeeee,
        DarkTurquoise = 0x00ced1,
        MediumTurquoise = 0x48d1cc,
        Turquoise = 0x40e0d0,
        Cyan = 0x00ffff,
        Aqua = 0x00ffff,
        LightCyan = 0xe0ffff,
        CadetBlue = 0x5f9ea0,
        MediumAquamarine = 0x66cdaa,
        Aquamarine = 0x7fffd4,
        DarkGreen = 0x006400,
        DarkOliveGreen = 0x556b2f,
        DarkSeaGreen = 0x8fbc8f,
        SeaGreen = 0x2e8b57,
        MediumSeaGreen = 0x3cb371,
        LightSeaGreen = 0x20b2aa,
        PaleGreen = 0x98fb98,
        SpringGreen = 0x00ff7f,
        LawnGreen = 0x7cfc00,
        Green = 0x00ff00,
        Lime = 0x00ff00,
        X11Green = 0x00ff00,
        WebGreen = 0x008000,
        Chartreuse = 0x7fff00,
        MediumSpringGreen = 0x00fa9a,
        GreenYellow = 0xadff2f,
        LimeGreen = 0x32cd32,
        YellowGreen = 0x9acd32,
        ForestGreen = 0x228b22,
        OliveDrab = 0x6b8e23,
        DarkKhaki = 0xbdb76b,
        Khaki = 0xf0e68c,
        PaleGoldenrod = 0xeee8aa,
        LightGoldenrodYellow = 0xfafad2,
        LightYellow = 0xffffe0,
        Yellow = 0xffff00,
        Gold = 0xffd700,
        LightGoldenrod = 0xeedd82,
        Goldenrod = 0xdaa520,
        DarkGoldenrod = 0xb8860b,
        RosyBrown = 0xbc8f8f,
        IndianRed = 0xcd5c5c,
        SaddleBrown = 0x8b4513,
        Sienna = 0xa0522d,
        Peru = 0xcd853f,
        Burlywood = 0xdeb887,
        Beige = 0xf5f5dc,
        Wheat = 0xf5deb3,
        SandyBrown = 0xf4a460,
        Tan = 0xd2b48c,
        Chocolate = 0xd2691e,
        Firebrick = 0xb22222,
        Brown = 0xa52a2a,
        DarkSalmon = 0xe9967a,
        Salmon = 0xfa8072,
        LightSalmon = 0xffa07a,
        Orange = 0xffa500,
        DarkOrange = 0xff8c00,
        Coral = 0xff7f50,
        LightCoral = 0xf08080,
        Tomato = 0xff6347,
        OrangeRed = 0xff4500,
        Red = 0xff0000,
        HotPink = 0xff69b4,
        DeepPink = 0xff1493,
        Pink = 0xffc0cb,
        LightPink = 0xffb6c1,
        PaleVioletRed = 0xdb7093,
        Maroon = 0xb03060,
        X11Maroon = 0xb03060,
        WebMaroon = 0x800000,
        MediumVioletRed = 0xc71585,
        VioletRed = 0xd02090,
        Magenta = 0xff00ff,
        Fuchsia = 0xff00ff,
        Violet = 0xee82ee,
        Plum = 0xdda0dd,
        Orchid = 0xda70d6,
        MediumOrchid = 0xba55d3,
        DarkOrchid = 0x9932cc,
        DarkViolet = 0x9400d3,
        BlueViolet = 0x8a2be2,
        Purple = 0xa020f0,
        X11Purple = 0xa020f0,
        WebPurple = 0x800080,
        MediumPurple = 0x9370db,
        Thistle = 0xd8bfd8,
        Snow1 = 0xfffafa,
        Snow2 = 0xeee9e9,
        Snow3 = 0xcdc9c9,
        Snow4 = 0x8b8989,
        Seashell1 = 0xfff5ee,
        Seashell2 = 0xeee5de,
        Seashell3 = 0xcdc5bf,
        Seashell4 = 0x8b8682,
        AntiqueWhite1 = 0xffefdb,
        AntiqueWhite2 = 0xeedfcc,
        AntiqueWhite3 = 0xcdc0b0,
        AntiqueWhite4 = 0x8b8378,
        Bisque1 = 0xffe4c4,
        Bisque2 = 0xeed5b7,
        Bisque3 = 0xcdb79e,
        Bisque4 = 0x8b7d6b,
        PeachPuff1 = 0xffdab9,
        PeachPuff2 = 0xeecbad,
        PeachPuff3 = 0xcdaf95,
        PeachPuff4 = 0x8b7765,
        NavajoWhite1 = 0xffdead,
        NavajoWhite2 = 0xeecfa1,
        NavajoWhite3 = 0xcdb38b,
        NavajoWhite4 = 0x8b795e,
        LemonChiffon1 = 0xfffacd,
        LemonChiffon2 = 0xeee9bf,
        LemonChiffon3 = 0xcdc9a5,
        LemonChiffon4 = 0x8b8970,
        Cornsilk1 = 0xfff8dc,
        Cornsilk2 = 0xeee8cd,
        Cornsilk3 = 0xcdc8b1,
        Cornsilk4 = 0x8b8878,
        Ivory1 = 0xfffff0,
        Ivory2 = 0xeeeee0,
        Ivory3 = 0xcdcdc1,
        Ivory4 = 0x8b8b83,
        Honeydew1 = 0xf0fff0,
        Honeydew2 = 0xe0eee0,
        Honeydew3 = 0xc1cdc1,
        Honeydew4 = 0x838b83,
        LavenderBlush1 = 0xfff0f5,
        LavenderBlush2 = 0xeee0e5,
        LavenderBlush3 = 0xcdc1c5,
        LavenderBlush4 = 0x8b8386,
        MistyRose1 = 0xffe4e1,
        MistyRose2 = 0xeed5d2,
        MistyRose3 = 0xcdb7b5,
        MistyRose4 = 0x8b7d7b,
        Azure1 = 0xf0ffff,
        Azure2 = 0xe0eeee,
        Azure3 = 0xc1cdcd,
        Azure4 = 0x838b8b,
        SlateBlue1 = 0x836fff,
        SlateBlue2 = 0x7a67ee,
        SlateBlue3 = 0x6959cd,
        SlateBlue4 = 0x473c8b,
        RoyalBlue1 = 0x4876ff,
        RoyalBlue2 = 0x436eee,
        RoyalBlue3 = 0x3a5fcd,
        RoyalBlue4 = 0x27408b,
        Blue1 = 0x0000ff,
        Blue2 = 0x0000ee,
        Blue3 = 0x0000cd,
        Blue4 = 0x00008b,
        DodgerBlue1 = 0x1e90ff,
        DodgerBlue2 = 0x1c86ee,
        DodgerBlue3 = 0x1874cd,
        DodgerBlue4 = 0x104e8b,
        SteelBlue1 = 0x63b8ff,
        SteelBlue2 = 0x5cacee,
        SteelBlue3 = 0x4f94cd,
        SteelBlue4 = 0x36648b,
        DeepSkyBlue1 = 0x00bfff,
        DeepSkyBlue2 = 0x00b2ee,
        DeepSkyBlue3 = 0x009acd,
        DeepSkyBlue4 = 0x00688b,
        SkyBlue1 = 0x87ceff,
        SkyBlue2 = 0x7ec0ee,
        SkyBlue3 = 0x6ca6cd,
        SkyBlue4 = 0x4a708b,
        LightSkyBlue1 = 0xb0e2ff,
        LightSkyBlue2 = 0xa4d3ee,
        LightSkyBlue3 = 0x8db6cd,
        LightSkyBlue4 = 0x607b8b,
        SlateGray1 = 0xc6e2ff,
        SlateGray2 = 0xb9d3ee,
        SlateGray3 = 0x9fb6cd,
        SlateGray4 = 0x6c7b8b,
        LightSteelBlue1 = 0xcae1ff,
        LightSteelBlue2 = 0xbcd2ee,
        LightSteelBlue3 = 0xa2b5cd,
        LightSteelBlue4 = 0x6e7b8b,
        LightBlue1 = 0xbfefff,
        LightBlue2 = 0xb2dfee,
        LightBlue3 = 0x9ac0cd,
        LightBlue4 = 0x68838b,
        LightCyan1 = 0xe0ffff,
        LightCyan2 = 0xd1eeee,
        LightCyan3 = 0xb4cdcd,
        LightCyan4 = 0x7a8b8b,
        PaleTurquoise1 = 0xbbffff,
        PaleTurquoise2 = 0xaeeeee,
        PaleTurquoise3 = 0x96cdcd,
        PaleTurquoise4 = 0x668b8b,
        CadetBlue1 = 0x98f5ff,
        CadetBlue2 = 0x8ee5ee,
        CadetBlue3 = 0x7ac5cd,
        CadetBlue4 = 0x53868b,
        Turquoise1 = 0x00f5ff,
        Turquoise2 = 0x00e5ee,
        Turquoise3 = 0x00c5cd,
        Turquoise4 = 0x00868b,
        Cyan1 = 0x00ffff,
        Cyan2 = 0x00eeee,
        Cyan3 = 0x00cdcd,
        Cyan4 = 0x008b8b,
        DarkSlateGray1 = 0x97ffff,
        DarkSlateGray2 = 0x8deeee,
        DarkSlateGray3 = 0x79cdcd,
        DarkSlateGray4 = 0x528b8b,
        Aquamarine1 = 0x7fffd4,
        Aquamarine2 = 0x76eec6,
        Aquamarine3 = 0x66cdaa,
        Aquamarine4 = 0x458b74,
        DarkSeaGreen1 = 0xc1ffc1,
        DarkSeaGreen2 = 0xb4eeb4,
        DarkSeaGreen3 = 0x9bcd9b,
        DarkSeaGreen4 = 0x698b69,
        SeaGreen1 = 0x54ff9f,
        SeaGreen2 = 0x4eee94,
        SeaGreen3 = 0x43cd80,
        SeaGreen4 = 0x2e8b57,
        PaleGreen1 = 0x9aff9a,
        PaleGreen2 = 0x90ee90,
        PaleGreen3 = 0x7ccd7c,
        PaleGreen4 = 0x548b54,
        SpringGreen1 = 0x00ff7f,
        SpringGreen2 = 0x00ee76,
        SpringGreen3 = 0x00cd66,
        SpringGreen4 = 0x008b45,
        Green1 = 0x00ff00,
        Green2 = 0x00ee00,
        Green3 = 0x00cd00,
        Green4 = 0x008b00,
        Chartreuse1 = 0x7fff00,
        Chartreuse2 = 0x76ee00,
        Chartreuse3 = 0x66cd00,
        Chartreuse4 = 0x458b00,
        OliveDrab1 = 0xc0ff3e,
        OliveDrab2 = 0xb3ee3a,
        OliveDrab3 = 0x9acd32,
        OliveDrab4 = 0x698b22,
        DarkOliveGreen1 = 0xcaff70,
        DarkOliveGreen2 = 0xbcee68,
        DarkOliveGreen3 = 0xa2cd5a,
        DarkOliveGreen4 = 0x6e8b3d,
        Khaki1 = 0xfff68f,
        Khaki2 = 0xeee685,
        Khaki3 = 0xcdc673,
        Khaki4 = 0x8b864e,
        LightGoldenrod1 = 0xffec8b,
        LightGoldenrod2 = 0xeedc82,
        LightGoldenrod3 = 0xcdbe70,
        LightGoldenrod4 = 0x8b814c,
        LightYellow1 = 0xffffe0,
        LightYellow2 = 0xeeeed1,
        LightYellow3 = 0xcdcdb4,
        LightYellow4 = 0x8b8b7a,
        Yellow1 = 0xffff00,
        Yellow2 = 0xeeee00,
        Yellow3 = 0xcdcd00,
        Yellow4 = 0x8b8b00,
        Gold1 = 0xffd700,
        Gold2 = 0xeec900,
        Gold3 = 0xcdad00,
        Gold4 = 0x8b7500,
        Goldenrod1 = 0xffc125,
        Goldenrod2 = 0xeeb422,
        Goldenrod3 = 0xcd9b1d,
        Goldenrod4 = 0x8b6914,
        DarkGoldenrod1 = 0xffb90f,
        DarkGoldenrod2 = 0xeead0e,
        DarkGoldenrod3 = 0xcd950c,
        DarkGoldenrod4 = 0x8b6508,
        RosyBrown1 = 0xffc1c1,
        RosyBrown2 = 0xeeb4b4,
        RosyBrown3 = 0xcd9b9b,
        RosyBrown4 = 0x8b6969,
        IndianRed1 = 0xff6a6a,
        IndianRed2 = 0xee6363,
        IndianRed3 = 0xcd5555,
        IndianRed4 = 0x8b3a3a,
        Sienna1 = 0xff8247,
        Sienna2 = 0xee7942,
        Sienna3 = 0xcd6839,
        Sienna4 = 0x8b4726,
        Burlywood1 = 0xffd39b,
        Burlywood2 = 0xeec591,
        Burlywood3 = 0xcdaa7d,
        Burlywood4 = 0x8b7355,
        Wheat1 = 0xffe7ba,
        Wheat2 = 0xeed8ae,
        Wheat3 = 0xcdba96,
        Wheat4 = 0x8b7e66,
        Tan1 = 0xffa54f,
        Tan2 = 0xee9a49,
        Tan3 = 0xcd853f,
        Tan4 = 0x8b5a2b,
        Chocolate1 = 0xff7f24,
        Chocolate2 = 0xee7621,
        Chocolate3 = 0xcd661d,
        Chocolate4 = 0x8b4513,
        Firebrick1 = 0xff3030,
        Firebrick2 = 0xee2c2c,
        Firebrick3 = 0xcd2626,
        Firebrick4 = 0x8b1a1a,
        Brown1 = 0xff4040,
        Brown2 = 0xee3b3b,
        Brown3 = 0xcd3333,
        Brown4 = 0x8b2323,
        Salmon1 = 0xff8c69,
        Salmon2 = 0xee8262,
        Salmon3 = 0xcd7054,
        Salmon4 = 0x8b4c39,
        LightSalmon1 = 0xffa07a,
        LightSalmon2 = 0xee9572,
        LightSalmon3 = 0xcd8162,
        LightSalmon4 = 0x8b5742,
        Orange1 = 0xffa500,
        Orange2 = 0xee9a00,
        Orange3 = 0xcd8500,
        Orange4 = 0x8b5a00,
        DarkOrange1 = 0xff7f00,
        DarkOrange2 = 0xee7600,
        DarkOrange3 = 0xcd6600,
        DarkOrange4 = 0x8b4500,
        Coral1 = 0xff7256,
        Coral2 = 0xee6a50,
        Coral3 = 0xcd5b45,
        Coral4 = 0x8b3e2f,
        Tomato1 = 0xff6347,
        Tomato2 = 0xee5c42,
        Tomato3 = 0xcd4f39,
        Tomato4 = 0x8b3626,
        OrangeRed1 = 0xff4500,
        OrangeRed2 = 0xee4000,
        OrangeRed3 = 0xcd3700,
        OrangeRed4 = 0x8b2500,
        Red1 = 0xff0000,
        Red2 = 0xee0000,
        Red3 = 0xcd0000,
        Red4 = 0x8b0000,
        DeepPink1 = 0xff1493,
        DeepPink2 = 0xee1289,
        DeepPink3 = 0xcd1076,
        DeepPink4 = 0x8b0a50,
        HotPink1 = 0xff6eb4,
        HotPink2 = 0xee6aa7,
        HotPink3 = 0xcd6090,
        HotPink4 = 0x8b3a62,
        Pink1 = 0xffb5c5,
        Pink2 = 0xeea9b8,
        Pink3 = 0xcd919e,
        Pink4 = 0x8b636c,
        LightPink1 = 0xffaeb9,
        LightPink2 = 0xeea2ad,
        LightPink3 = 0xcd8c95,
        LightPink4 = 0x8b5f65,
        PaleVioletRed1 = 0xff82ab,
        PaleVioletRed2 = 0xee799f,
        PaleVioletRed3 = 0xcd6889,
        PaleVioletRed4 = 0x8b475d,
        Maroon1 = 0xff34b3,
        Maroon2 = 0xee30a7,
        Maroon3 = 0xcd2990,
        Maroon4 = 0x8b1c62,
        VioletRed1 = 0xff3e96,
        VioletRed2 = 0xee3a8c,
        VioletRed3 = 0xcd3278,
        VioletRed4 = 0x8b2252,
        Magenta1 = 0xff00ff,
        Magenta2 = 0xee00ee,
        Magenta3 = 0xcd00cd,
        Magenta4 = 0x8b008b,
        Orchid1 = 0xff83fa,
        Orchid2 = 0xee7ae9,
        Orchid3 = 0xcd69c9,
        Orchid4 = 0x8b4789,
        Plum1 = 0xffbbff,
        Plum2 = 0xeeaeee,
        Plum3 = 0xcd96cd,
        Plum4 = 0x8b668b,
        MediumOrchid1 = 0xe066ff,
        MediumOrchid2 = 0xd15fee,
        MediumOrchid3 = 0xb452cd,
        MediumOrchid4 = 0x7a378b,
        DarkOrchid1 = 0xbf3eff,
        DarkOrchid2 = 0xb23aee,
        DarkOrchid3 = 0x9a32cd,
        DarkOrchid4 = 0x68228b,
        Purple1 = 0x9b30ff,
        Purple2 = 0x912cee,
        Purple3 = 0x7d26cd,
        Purple4 = 0x551a8b,
        MediumPurple1 = 0xab82ff,
        MediumPurple2 = 0x9f79ee,
        MediumPurple3 = 0x8968cd,
        MediumPurple4 = 0x5d478b,
        Thistle1 = 0xffe1ff,
        Thistle2 = 0xeed2ee,
        Thistle3 = 0xcdb5cd,
        Thistle4 = 0x8b7b8b,
        Gray0 = 0x000000,
        Grey0 = 0x000000,
        Gray1 = 0x030303,
        Grey1 = 0x030303,
        Gray2 = 0x050505,
        Grey2 = 0x050505,
        Gray3 = 0x080808,
        Grey3 = 0x080808,
        Gray4 = 0x0a0a0a,
        Grey4 = 0x0a0a0a,
        Gray5 = 0x0d0d0d,
        Grey5 = 0x0d0d0d,
        Gray6 = 0x0f0f0f,
        Grey6 = 0x0f0f0f,
        Gray7 = 0x121212,
        Grey7 = 0x121212,
        Gray8 = 0x141414,
        Grey8 = 0x141414,
        Gray9 = 0x171717,
        Grey9 = 0x171717,
        Gray10 = 0x1a1a1a,
        Grey10 = 0x1a1a1a,
        Gray11 = 0x1c1c1c,
        Grey11 = 0x1c1c1c,
        Gray12 = 0x1f1f1f,
        Grey12 = 0x1f1f1f,
        Gray13 = 0x212121,
        Grey13 = 0x212121,
        Gray14 = 0x242424,
        Grey14 = 0x242424,
        Gray15 = 0x262626,
        Grey15 = 0x262626,
        Gray16 = 0x292929,
        Grey16 = 0x292929,
        Gray17 = 0x2b2b2b,
        Grey17 = 0x2b2b2b,
        Gray18 = 0x2e2e2e,
        Grey18 = 0x2e2e2e,
        Gray19 = 0x303030,
        Grey19 = 0x303030,
        Gray20 = 0x333333,
        Grey20 = 0x333333,
        Gray21 = 0x363636,
        Grey21 = 0x363636,
        Gray22 = 0x383838,
        Grey22 = 0x383838,
        Gray23 = 0x3b3b3b,
        Grey23 = 0x3b3b3b,
        Gray24 = 0x3d3d3d,
        Grey24 = 0x3d3d3d,
        Gray25 = 0x404040,
        Grey25 = 0x404040,
        Gray26 = 0x424242,
        Grey26 = 0x424242,
        Gray27 = 0x454545,
        Grey27 = 0x454545,
        Gray28 = 0x474747,
        Grey28 = 0x474747,
        Gray29 = 0x4a4a4a,
        Grey29 = 0x4a4a4a,
        Gray30 = 0x4d4d4d,
        Grey30 = 0x4d4d4d,
        Gray31 = 0x4f4f4f,
        Grey31 = 0x4f4f4f,
        Gray32 = 0x525252,
        Grey32 = 0x525252,
        Gray33 = 0x545454,
        Grey33 = 0x545454,
        Gray34 = 0x575757,
        Grey34 = 0x575757,
        Gray35 = 0x595959,
        Grey35 = 0x595959,
        Gray36 = 0x5c5c5c,
        Grey36 = 0x5c5c5c,
        Gray37 = 0x5e5e5e,
        Grey37 = 0x5e5e5e,
        Gray38 = 0x616161,
        Grey38 = 0x616161,
        Gray39 = 0x636363,
        Grey39 = 0x636363,
        Gray40 = 0x666666,
        Grey40 = 0x666666,
        Gray41 = 0x696969,
        Grey41 = 0x696969,
        Gray42 = 0x6b6b6b,
        Grey42 = 0x6b6b6b,
        Gray43 = 0x6e6e6e,
        Grey43 = 0x6e6e6e,
        Gray44 = 0x707070,
        Grey44 = 0x707070,
        Gray45 = 0x737373,
        Grey45 = 0x737373,
        Gray46 = 0x757575,
        Grey46 = 0x757575,
        Gray47 = 0x787878,
        Grey47 = 0x787878,
        Gray48 = 0x7a7a7a,
        Grey48 = 0x7a7a7a,
        Gray49 = 0x7d7d7d,
        Grey49 = 0x7d7d7d,
        Gray50 = 0x7f7f7f,
        Grey50 = 0x7f7f7f,
        Gray51 = 0x828282,
        Grey51 = 0x828282,
        Gray52 = 0x858585,
        Grey52 = 0x858585,
        Gray53 = 0x878787,
        Grey53 = 0x878787,
        Gray54 = 0x8a8a8a,
        Grey54 = 0x8a8a8a,
        Gray55 = 0x8c8c8c,
        Grey55 = 0x8c8c8c,
        Gray56 = 0x8f8f8f,
        Grey56 = 0x8f8f8f,
        Gray57 = 0x919191,
        Grey57 = 0x919191,
        Gray58 = 0x949494,
        Grey58 = 0x949494,
        Gray59 = 0x969696,
        Grey59 = 0x969696,
        Gray60 = 0x999999,
        Grey60 = 0x999999,
        Gray61 = 0x9c9c9c,
        Grey61 = 0x9c9c9c,
        Gray62 = 0x9e9e9e,
        Grey62 = 0x9e9e9e,
        Gray63 = 0xa1a1a1,
        Grey63 = 0xa1a1a1,
        Gray64 = 0xa3a3a3,
        Grey64 = 0xa3a3a3,
        Gray65 = 0xa6a6a6,
        Grey65 = 0xa6a6a6,
        Gray66 = 0xa8a8a8,
        Grey66 = 0xa8a8a8,
        Gray67 = 0xababab,
        Grey67 = 0xababab,
        Gray68 = 0xadadad,
        Grey68 = 0xadadad,
        Gray69 = 0xb0b0b0,
        Grey69 = 0xb0b0b0,
        Gray70 = 0xb3b3b3,
        Grey70 = 0xb3b3b3,
        Gray71 = 0xb5b5b5,
        Grey71 = 0xb5b5b5,
        Gray72 = 0xb8b8b8,
        Grey72 = 0xb8b8b8,
        Gray73 = 0xbababa,
        Grey73 = 0xbababa,
        Gray74 = 0xbdbdbd,
        Grey74 = 0xbdbdbd,
        Gray75 = 0xbfbfbf,
        Grey75 = 0xbfbfbf,
        Gray76 = 0xc2c2c2,
        Grey76 = 0xc2c2c2,
        Gray77 = 0xc4c4c4,
        Grey77 = 0xc4c4c4,
        Gray78 = 0xc7c7c7,
        Grey78 = 0xc7c7c7,
        Gray79 = 0xc9c9c9,
        Grey79 = 0xc9c9c9,
        Gray80 = 0xcccccc,
        Grey80 = 0xcccccc,
        Gray81 = 0xcfcfcf,
        Grey81 = 0xcfcfcf,
        Gray82 = 0xd1d1d1,
        Grey82 = 0xd1d1d1,
        Gray83 = 0xd4d4d4,
        Grey83 = 0xd4d4d4,
        Gray84 = 0xd6d6d6,
        Grey84 = 0xd6d6d6,
        Gray85 = 0xd9d9d9,
        Grey85 = 0xd9d9d9,
        Gray86 = 0xdbdbdb,
        Grey86 = 0xdbdbdb,
        Gray87 = 0xdedede,
        Grey87 = 0xdedede,
        Gray88 = 0xe0e0e0,
        Grey88 = 0xe0e0e0,
        Gray89 = 0xe3e3e3,
        Grey89 = 0xe3e3e3,
        Gray90 = 0xe5e5e5,
        Grey90 = 0xe5e5e5,
        Gray91 = 0xe8e8e8,
        Grey91 = 0xe8e8e8,
        Gray92 = 0xebebeb,
        Grey92 = 0xebebeb,
        Gray93 = 0xededed,
        Grey93 = 0xededed,
        Gray94 = 0xf0f0f0,
        Grey94 = 0xf0f0f0,
        Gray95 = 0xf2f2f2,
        Grey95 = 0xf2f2f2,
        Gray96 = 0xf5f5f5,
        Grey96 = 0xf5f5f5,
        Gray97 = 0xf7f7f7,
        Grey97 = 0xf7f7f7,
        Gray98 = 0xfafafa,
        Grey98 = 0xfafafa,
        Gray99 = 0xfcfcfc,
        Grey99 = 0xfcfcfc,
        Gray100 = 0xffffff,
        Grey100 = 0xffffff,
        DarkGrey = 0xa9a9a9,
        DarkGray = 0xa9a9a9,
        DarkBlue = 0x00008b,
        DarkCyan = 0x008b8b,
        DarkMagenta = 0x8b008b,
        DarkRed = 0x8b0000,
        LightGreen = 0x90ee90,
        Crimson = 0xdc143c,
        Indigo = 0x4b0082,
        Olive = 0x808000,
        RebeccaPurple = 0x663399,
        Silver = 0xc0c0c0,
        Teal = 0x008080,
    };
}