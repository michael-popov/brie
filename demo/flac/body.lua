if counter >= 100 then
    counter = 1
    println("Another 100 passed. Overall progress: %d", overall_count)
end
counter = counter + 1
overall_count = overall_count + 1

t = read_header()
if t == false then
    add_invalid_data(BRIE_PATH)
    return
end

dir, name = split_path(BRIE_PATH)
if name == nil then
    name = BRIE_PATH
end

if dir == nil then
    dir = 0
end

item = {}
item.album = 0
item.genre = 0
item.artist = 0
item.year = 0
item.title = ''
item.track = 0
item.name = name
item.dir = add_dir(dir)

while true do
    s = read_tag()
    if s == '' then
        break
    end

    for name in string.gmatch(s, "(%w+)=") do
        value = string.sub(s, #name+2, #s)

        name = string.upper(name)

        if name == 'ARTIST' or name == 'ALBUMARTIST' then
            item.artist = add_artist(value)
        elseif name == 'YEAR' or name == 'DATE' then
            local val = tonumber(value)
            if val ~= nil then
                item.year = val
            end
        elseif name == 'GENRE' then
            item.genre = add_genre(value)
        elseif name == 'TITLE' then
            item.title = value
        elseif name == 'ALBUM' then
            item.album = add_album(value)
        elseif name == 'TRACKNUMBER' then
            local val = tonumber(value)
            if val ~= nil then
                item.track = val
            end
        end

    end
end

add_item(item)



