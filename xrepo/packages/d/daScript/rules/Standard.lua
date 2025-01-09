rule("Standard")
    after_build(function (target)
        import("skr.utils")

        local outdir = target:extraconf("rules", "@daScript/Standard", "outdir") or "./"
        if not path.is_absolute(outdir) then
            outdir = path.join(target:targetdir(), outdir)
        end

        local daslibdir = path.join(os.scriptdir(), "..", "daslib")
        local dasliboutdir = path.join(outdir, "daslib")

        local dastestdir = path.join(os.scriptdir(), "..", "dastest")
        local dastestoutdir = path.join(outdir, "dastest")

        local dasTool = path.join(os.scriptdir(), "..", "bin")
        local dasToolOut = path.join(outdir, "bin")

        local depend = import("core.project.depend")
        if utils.is_changed(target:dependfile(daslibdir), {daslibdir, dasliboutdir, target:targetfile()}) then
            os.vcp(daslibdir, outdir)
        end

        if utils.is_changed(target:dependfile(dastestdir), {dastestdir, dastestoutdir, target:targetfile()}) then
            os.vcp(dastestdir, outdir)
        end

        if not os.exists(dasToolOut) then
            if utils.is_changed(target:dependfile(dasTool), {dasTool, dasToolOut, target:targetfile()}) then
                os.vcp(dasTool, outdir)
            end
        end
        
        print("daslib and dastest updated to ./"..path.relative(outdir, os.projectdir()).."!")
    end)