_skr_env_table = {}
_skr_env_committed = false

-- set env
function skr_env_set(key, value)
    _skr_env_table[key] = value
end

-- add env (trait value as list)
function skr_env_add(key, value)
    -- collect old value
    local old_value = _skr_env_table[key]
    if not old_value then
        old_value = {}
    elseif type(old_value) ~= "table" then
        old_value = { old_value }
    end

    -- insert new value
    table.insert(old_value, value)

    -- setup value
    _skr_env_table[key] = old_value
end

-- get env
function skr_env_get(key)
    return _skr_env_table[key]
end

-- commit env into skr global target
function skr_env_commit()
    if _skr_env_committed then
        cprint("${red} call 'skr_env_commit' after committed${clear}")
        exit(-1)
    end

    skr_global_target()
    set_values("skr.env", table.wrap_lock(_skr_env_table),{
        force = true,
        expand = false,
    })
    skr_global_target_end()
end