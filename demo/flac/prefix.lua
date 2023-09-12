
Items = {}
InvalidData = {}

Genres = {}
Artists = {}
Albums = {}
Dirs = {}

GenresIds = {}
ArtistsIds = {}
AlbumsIds = {}
DirsIds = {}

ItemKey = 1
GenreKey = 1
ArtistKey = 1
AlbumKey = 1
DirKey = 1

function is_flac()
    local s = read('str#4')
    if s == 'fLaC' then
        return true
    end
    return false
end

function find_marker()
    local n = find('Mutagen', 4096)
    if (n < 0) then
        n = find('reference', 4096)
    end
    return n
end

function read_tag()
    local n = read('u32')
    if n > 255 or n < 1 then
        return ''
    end

    local fmt = string.format('str#%d', n)
    s = read(fmt)
    return s
end

function read_header()
    if is_flac() == false then
        return false
    end

    local n = find_marker()
    if n < 0 then
        n = find('TITLE', 4096)
        if n > 0 then
            setpos(n)
            return true
        end

        return false
    end

    setpos(n - 4)
    local s = read_tag()
    if s == '' then
        print("Bad reference")
        return false
    end

    -- dummy 4 bytes
    read('u8*4')

    return true
end

function add_artist_key(name)
    if ArtistsIds[name] == nil then
        local key = ArtistKey
        ArtistsIds[name] = key
        ArtistKey = ArtistKey + 1
        return key
    end

    return ArtistsIds[name]
end

function add_artist(name)
    local key = add_artist_key(name)
    if Artists[key] == nil then
        Artists[key] = name
    end
    return key
end

function add_album_key(name)
    if AlbumsIds[name] == nil then
        local key = AlbumKey
        AlbumsIds[name] = key
        AlbumKey = AlbumKey + 1
        return key
    end

    return AlbumsIds[name]
end

function add_album(name)
    local key = add_album_key(name)
    if Albums[key] == nil then
        Albums[key] = name
    end
    return key
end

function add_genre_key(name)
    if GenresIds[name] == nil then
        local key = GenreKey
        GenresIds[name] = key
        GenreKey = GenreKey + 1
        return key
    end

    return GenresIds[name]
end

function add_genre(name)
    local key = add_genre_key(name)
    if Genres[key] == nil then
        Genres[key] = name
    end
    return key
end

function add_item(item)
    Items[ItemKey] = item
    ItemKey = ItemKey + 1
end

function get_directory(path)
    local last = 0
    local qqq = 0
    while true do
        qqq = string.find(path, "/", qqq+1)
        if qqq == nil then
            break
        end
        last = qqq
    end

    return string.sub(path, 1, last-1)
end

function add_invalid_data(path)
    local dir = get_directory(path)
    InvalidData[dir] = 1
end

function add_dir_key(name)
    if DirsIds[name] == nil then
        local key = DirKey
        DirsIds[name] = key
        DirKey = DirKey + 1
        return key
    end

    return DirsIds[name]
end

function add_dir(dir)
    local key = add_dir_key(dir)
    if Dirs[key] == nil then
        Dirs[key] = dir
    end
    return key
end

function split_path(path)
    local i = 1
    local p = 0
    local qqq = {}
    while true do
        p = string.find(path, "/", p+1)
        if p == nil then
            break
        end
        qqq[i] = p
        i = i + 1
    end

    local n = #qqq
    if n < 3 then
        return 0
    end

    local dir = string.sub(path, qqq[4]+1, qqq[n] - 1)
    local name = string.sub(path, qqq[n]+1, #path)

    return dir, name
end

counter = 1
overall_count = 1


