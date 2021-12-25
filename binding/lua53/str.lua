local M = {}

function M.to_tbl(src)
    return load("return "..src)()
end

local to_str_ex = function(v)
    local tv = type(v)
    if     tv == 'table' then
        return M.from_tbl(v)
    elseif tv == 'string' then
        return "\""..v.."\""
    else
        return tostring(v)
    end
end
function M.from_tbl(tbl)
    if not tbl then
        return ""
    end

    local bak = "{"
    local i   = 1
    local sig = ""
    for k, v in pairs(tbl) do
        local tk   = type(k)
        local strv = to_str_ex(v)
        if strv then
            if      k == i then
                bak = bak..sig..strv
            elseif tk == 'number' or tk == 'string' then
                bak = bak..sig.."["..to_str_ex(k).."] = "..strv
            elseif tk == 'userdata' then
                bak = bak..sig.."*s"..M.from_tbl(getmetatable(k)).."*e".." = "..strv
            else
                bak = bak..sig..k.." = "..strv
            end
        end
        i   = i + 1
        sig = ", "
    end
    return bak.."}"
end

function M.rand(len, ...)
    local args = {...}
    local seed = 0
    if args[1] then
        seed = args[1]
    else
        seed = tostring(os.clock()):reverse():sub(1, 6)
    end

    math.randomseed(seed)
    local bak = ""
    for i = 1, len do
        local randNum = math.random(1,3)
        if     randNum == 1  then
            randNum = string.char(math.random(0, 26) + 65)
        elseif randNum == 2 then
            randNum = string.char(math.random(0, 26) + 97)
        else
            randNum = math.random(0,9)
        end
        bak = bak..randNum
    end
    return bak
end

function M.split(src, sep, ...)
    local bak   = {}
    local begin = 0
    local fin   = 0
    while src do
        begin, fin = string.find(src, sep, 1, true)
        if not begin then
            table.insert(bak, src)
            break
        end
        if begin ~= 1 then
            table.insert(bak, string.sub(src, 1, begin - 1))
        end
        src = string.sub(src, fin+1)
    end
    return bak
end

function M.sprintf(fmt, ...)
    local args = {...}

    for i, v in ipairs(args) do
        local tv    = type(v)
        if tv == 'table' then
            args[i] = M.from_tbl(v)
        else
            args[i] = tostring(v)
        end
    end

    return string.format(fmt, table.unpack(args))
end

function M.printf(printers, fmt, ...)
    if not printers then
        return
    end
    for _, p in ipairs(printers) do
        p(M.sprintf(fmt, ...))
    end
end

return M