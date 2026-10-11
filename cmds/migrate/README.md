# migrate

Applies, rolls back and reports database migrations.

```sh
zuri migrate [up | status | down | reset | new <name>...] [options]
```

A migration is a Zuri file in the project's `migrations` directory,
named with a version and what it does, such as
`20261011143000_create_posts.zu`. It exports `up(db)`, which makes the
change, and `down(db)`, which undoes it, both written with `db.schema`
so the same file migrates SQLite, PostgreSQL, MySQL and MariaDB. `db`
is the migration's `Transaction`, or the `Connection` where it runs
outside one, as it always does on MySQL and MariaDB:

```zuri
import sql { Blueprint, Connection, Transaction }

def up(db: Transaction|Connection) {
  db.schema.create_table('posts', @(table: Blueprint) {
    table.id()
    table.string('title', 200)
    table.timestamps()
  })
}

def down(db: Transaction|Connection) {
  db.schema.drop_table('posts')
}
```

Each migration runs once, in order, in a transaction of its own on the
engines that roll a schema change back, and is recorded in
`zuri_migrations`. Two processes migrating one database at once apply
each migration once. `sql.migrate` is the same thing as a library, for
a program that migrates its own database as it starts.

The project is the nearest directory above the working directory with
a `project.toml`, or the working directory when there is none. The
database is `--database`, or `DATABASE_URL`, from the environment or
from the project's `.env`.

| Command | What it does |
| --- | --- |
| `zuri migrate` | applies every pending migration |
| `zuri migrate up` | the same; `--steps` and `--to` stop sooner |
| `zuri migrate status` | lists every migration and whether it has run |
| `zuri migrate down` | rolls back the latest batch; `--steps` and `--to` choose otherwise |
| `zuri migrate reset` | rolls back every migration |
| `zuri migrate new <name>...` | writes a new, empty migration for each name |

A batch is what one `up` applied. `status` marks a migration whose
file changed after it ran, and one that ran but has no file any more.

A name is letters, digits and underscores. `new create_posts` writes a
migration creating and dropping a `posts` table; any other name writes
one with empty `up` and `down`. Either way they start typed, as above.
`new create_posts add_slug_to_posts` writes two, in that order.

## Flags

| Flag | What it does |
| --- | --- |
| `-d, --database <url>` | The connection string. |
| `--dir <path>` | The directory of migrations; `migrations` in the project by default. |
| `--table <name>` | The table recording migrations; `zuri_migrations` by default. |
| `-s, --steps <count>` | With `up` or `down`, how many migrations at most. |
| `--to <name>` | With `up`, the last migration to apply; with `down`, the one to leave latest. |
