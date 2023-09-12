function print_table(title, t)
    f = io.open(title, "w")
    io.output(f)
    for val, n in pairs(t) do
      io.write(val)
      io.write("\n")
    end
    io.close(f)
end

function table_to_sql(title, t)
    local file_name = string.format('%s.sql', title)
    f = io.open(file_name, "w")
    io.output(f)

    local drop_stmt = string.format('DROP TABLE IF EXISTS %s;\n', title)
    local create_stmt = string.format('CREATE TABLE %s (id INTEGER PRIMARY KEY, name TEXT) WITHOUT ROWID;\n',
        title)
    io.write(drop_stmt)
    io.write(create_stmt)

    for val, n in pairs(t) do
        val = string.gsub(val, "\"", " ")
        local insert_stmt = string.format("INSERT INTO %s VALUES (%d, \"%s\");\n", title, n, val)
        io.write(insert_stmt)
    end
    io.close(f)
end

function items_to_sql()
    local file_name = 'items.sql'
    f = io.open(file_name, "w")
    io.output(f)

    local drop_stmt = 'DROP TABLE IF EXISTS items;\n'
    local create_stmt = 'CREATE TABLE items (id INTEGER PRIMARY KEY, ' ..
                                            'album INTEGER, ' ..
                                            'genre INTEGER, ' ..
                                            'artist INTEGER, ' ..
                                            'year INTEGER, ' ..
                                            'title TEXT, ' ..
                                            'track INTEGER, ' ..
                                            'name TEXT, ' ..
                                            'dir INTEGER);\n'

    io.write(drop_stmt)
    io.write(create_stmt)

    local i = 1
    for i = 1, #Items do
        local q = Items[i]

        if q.album == nil then
            print(i, "Album is nil")
            goto continue
        end

        if q.genre == nil then
            print(i, "Genre is nil")
            goto continue
        end

        if q.artist == nil then
            print(i, "Artist is nil")
            goto continue
        end

        if q.year == nil then
            print(i, "year is nil")
            goto continue
        end

        if q.title == nil then
            print(i, "title is nil")
            goto continue
        end
        q.title = string.gsub(q.title, "\"", " ")

        if q.track == nil then
            print(i, "track is nil")
            goto continue
        end

        if q.name == nil then
            print(i, "name is nil")
            goto continue
        end
        q.name = string.gsub(q.name, "\"", " ")

        if q.dir == nil then
            print(i, "dir is nil")
            goto continue
        end

        local s = string.format("INSERT INTO items VALUES (%d, %d, %d, %d, %d, \"%s\", %d, \"%s\", %d);\n",
            i, q.album, q.genre, q.artist, q.year, q.title, q.track, q.name, q.dir)
        io.write(s)

        ::continue::
    end

    io.close(f)

end

print_table("InvalidData", InvalidData)

table_to_sql("genres", GenresIds)
table_to_sql("artists", ArtistsIds)
table_to_sql("albums", AlbumsIds)
table_to_sql("dirs", DirsIds)
items_to_sql()

println("Completed %d titles", #Items)

