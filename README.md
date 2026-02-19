# Project VI Webserver - Super Sorter 3400
CROW-based webserver that runs a simple puzzle game. It runs in a Ubuntu container, so it's pretty easy to set up!

## How to Build & Run
1. Clone the repo

If you have a build folder in your repo, delete it before proceeding!

2. Set up the `.env` file (explained more under "Connecting to the Database")
3. Ensure [Docker](https://www.docker.com/) is installed and running
4. Open a terminal window at the project root, and run:

```
docker compose up --build
```

This should build and run the server all in one go.

5. Open your browser at `http://localhost:18080`

If all is working correctly, you should see the home page in your browser.

## Connecting to the Database
Previously, this was configured through the `db.conf` file. As part of containerization, this has been replaced with a `.env` file. Due to the sensitive nature of the information, a `.env.example` file has been committed in its place. To set this up for connection:
1. Create a `.env` file in the same directory as the example file (or, just rename the example file to `.env`).
2. Populate the `.env` file with the correct information. For example:

```
DB_SERVER=SERVER_NAME.database.windows.net

"SERVER_NAME" should be replaced with the actual name of the server.
```

Please do **NOT** commit the `.env` file directly!!
