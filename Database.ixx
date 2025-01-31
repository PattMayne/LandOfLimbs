/**
*  ____        _        _
* |  _ \  __ _| |_ __ _| |__   __ _ ___  ___
* | | | |/ _` | __/ _` | '_ \ / _` / __|/ _ \
* | |_| | (_| | || (_| | |_) | (_| \__ \  __/
* |____/ \__,_|\__\__,_|_.__/ \__,_|___/\___|
* 
* 
* The modules for objects which are saved to the database do not import the database themselves.
* Instead, the screen modules which load those classes also load the database module.
* The database module also loads all the modules whose classes must be saved to the database.
* So Limb does not have a save() function.
* Instead, the database will have a save() function for each class,
* and the screen modules can call THAT function when needed.
* The same process applies for getting data from the database,
* and using that data to create objects.
*/


module;
export module Database;

import <vector>;
import <cstdlib>;
import <iostream>;
import <unordered_map>;
import <fstream>;
import <string>;
#include "sqlite3.h"

import CharacterClasses;
import MapClasses;
import TypeStorage;
import LimbFormMasterList;
import FormFactory;
import GameState;

using namespace std;

const char* dbPath() { return "data/limbs_data.db"; }

export string stringFromUnsignedChar(const unsigned char* c_str) {
    std::string str;
    int i{ 0 };
    while (c_str[i] != '\0') { str += c_str[i++]; }
    return str;
}

/* Creates the database and tables if they don't exist. */
export void createDB() {
    sqlite3* db;

    /* Open database (create if does not exist). */
    int dbFailed = sqlite3_open(dbPath(), &db);
    cout << dbFailed << "\n";
    if (dbFailed == 0) {
        cout << "Opened Database Successfully." << "\n";

        /* Get the schema file. */
        string schemaFilename = "data/schema.sql";
        ifstream schemaFile(schemaFilename);

        if (schemaFile.is_open()) {
            cout << "Opened schema file.\n";

            /* Get the SQL string from the open file. */
            string sql((istreambuf_iterator<char>(schemaFile)), istreambuf_iterator<char>());
            char* errMsg = nullptr;
            int returnCode = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);

            if (returnCode == SQLITE_OK) {  cout << "Schema executed successfully." << endl; }
            else {
                cerr << "SQL error: " << errMsg << endl;
                sqlite3_free(errMsg); }
        }
        else {
            cerr << "Could not open file: " << schemaFilename << endl;
            return;
        }
    }
    else { cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl; }

    /* Close database. */
    sqlite3_close(db);
}

/*
* When a new Limb is created on a Map object, use this function to create the limb in the database.
* Then the ID is sent back to create the actual Limb.
*/
export int createRoamingLimb(Limb& limb, string mapSlug, Point position) {
    sqlite3* db;
    int id = -1;

    /* Open database (create if does not exist). */
    int dbFailed = sqlite3_open(dbPath(), &db);
    cout << dbFailed << "\n";
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return id;
    }

    /* Create statement for adding new Limb object to the database. */
    const char* insertSQL = "INSERT INTO LIMB (form_slug, map_slug, position_x, position_y) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* statement;

    /* Prepare the statement. */
    int returnCode = sqlite3_prepare_v2(db, insertSQL, -1, &statement, nullptr);
    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return id;
    }

    string limbFormSlugString = limb.getForm().slug;

    /* Bind the data. */
    sqlite3_bind_text(statement, 1, limbFormSlugString.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(statement, 2, mapSlug.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(statement, 3, position.x);
    sqlite3_bind_int(statement, 4, position.y);

    /* Execute the statement. */
    returnCode = sqlite3_step(statement);
    if (returnCode != SQLITE_DONE) { cerr << "Insert failed: " << sqlite3_errmsg(db) << endl;  }
    else {
        /* Get the ID of the saved item. */
        id = static_cast<int>(sqlite3_last_insert_rowid(db));
    }

    /* Finalize statement and close database. */
    sqlite3_finalize(statement);
    sqlite3_close(db);

    return id;
}

/* When a limb changes owner. */
export bool updateLimbOwner(int limbID, int newCharacterID) {
    bool success = false;
    sqlite3* db;

    /* Open database. */
    int dbFailed = sqlite3_open(dbPath(), &db);
    cout << dbFailed << "\n";
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return success;
    }

    /* No need to change the map_slug because map only loads non-owned limbs. */
    const char* updateSQL = "UPDATE limb SET character_id = ? WHERE id = ?;";
    sqlite3_stmt* statement;

    /* Prepare the statement. */
    int returnCode = sqlite3_prepare_v2(db, updateSQL, -1, &statement, nullptr);
    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare LIMB UPDATE statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return success;
    }

    /* Bind the values. */
    sqlite3_bind_int(statement, 1, newCharacterID);
    sqlite3_bind_int(statement, 2, limbID);

    cout << "Updated Limb ID: " << limbID << "\n";

    /* Execute the statement. */
    returnCode = sqlite3_step(statement);
    if (returnCode != SQLITE_DONE) { cerr << "Update failed: " << sqlite3_errmsg(db) << endl; }
    else { success = true; }

    /* Finalize statement and close database. */
    sqlite3_finalize(statement);
    sqlite3_close(db);

    return success;
}


export bool createNewMap(Map& map) {
    bool success = false;
    sqlite3* db;
    string slugString = map.getForm().slug;
    const char* mapSlug = slugString.c_str();
    int playerX = map.getPlayerCharacter().getBlockX();
    int playerY = map.getPlayerCharacter().getBlockY();

    /* Open database. */
    int dbFailed = sqlite3_open(dbPath(), &db);
    cout << dbFailed << "\n";
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return success;
    }

    /* First save the Map object to the DB. */

    /* Create statement for adding new Map object to the database. */
    const char* insertSQL = "INSERT INTO map (slug, character_x, character_y) VALUES (?, ?, ?);";
    sqlite3_stmt* statement;

    /* Prepare the statement. */
    int returnCode = sqlite3_prepare_v2(db, insertSQL, -1, &statement, nullptr);
    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare MAP insert statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return success;
    }

    /* Bind the data and execute the statement. */
    sqlite3_bind_text(statement, 1, mapSlug, -1, SQLITE_STATIC);
    sqlite3_bind_int(statement, 2, playerX);
    sqlite3_bind_int(statement, 3, playerY);
    returnCode = sqlite3_step(statement);

    if (returnCode != SQLITE_DONE) {
        cerr << "Insert failed for MAP: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(statement);
        sqlite3_close(db);
        return success;
    }

    /* Finalize statement. */
    sqlite3_finalize(statement);


    /* 
    * Now run a loop to save each block.
    * Do a Transaction to reduce time.
    */

    /* Create statement for adding new Block object to the database. */
    const char* insertBlockSQL = "INSERT INTO block (map_slug, position_x, position_y, is_floor, is_path) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* blockStatement;

    /* Prepare the statement before starting the loop. */
    returnCode = sqlite3_prepare_v2(db, insertBlockSQL, -1, &blockStatement, nullptr);
    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare BLOCK insert statement: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    char* errMsg;

    /* Begin the transaction. */
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    bool blockError = false;
    int isFloor = 0;
    int isPath = 0;

    for (int y = 0; y < map.getRows().size(); ++y) {
        vector<Block>& row = map.getRows()[y];
        for (int x = 0; x < row.size(); ++x) {
            Block& block = row[x];

            /* save this particular block to the database. */

            isFloor = block.getIsFloor() ? 1 : 0;
            isPath = block.getIsPath() ? 1 : 0;
            /* Bind the data and execute the statement. */
            sqlite3_bind_text(blockStatement, 1, mapSlug, -1, SQLITE_STATIC);
            sqlite3_bind_int(blockStatement, 2, x);
            sqlite3_bind_int(blockStatement, 3, y);
            sqlite3_bind_int(blockStatement, 4, isFloor);
            sqlite3_bind_int(blockStatement, 5, isPath);

            if (sqlite3_step(blockStatement) == SQLITE_DONE) {
                /* Get the ID of the saved item. */
                int blockID = static_cast<int>(sqlite3_last_insert_rowid(db));
                block.setId(blockID);
            }
            else {
                cerr << "Insert failed for BLOCK: " << sqlite3_errmsg(db) << endl;
                blockError = true;
                break;
            }

            sqlite3_reset(blockStatement);
        }
        if (blockError) {
            cout << "ERROR\n";
            break;
        }
    }

    /* Finalize the statement, commit the transaction. */
    sqlite3_finalize(blockStatement);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);

    if (blockError) { 
        /* DELETE map and all blocks */
        sqlite3_close(db);
        return success;
    }

    cout << map.getRoamingLimbs().size() << " roaming limbs.\n";

    /* Next do a transaction to save all the Roaming Limbs to the database. */

    /* Create statement for adding new Limb object to the database. */
    const char* insertLimbSQL = "INSERT INTO limb (form_slug, map_slug, position_x, position_y) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* limbStatement;

    /* Prepare the statement before starting the loop. */
    returnCode = sqlite3_prepare_v2(db, insertLimbSQL, -1, &limbStatement, nullptr);
    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare LIMB insert statement: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    /* Begin the transaction. */
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    bool limbError = false;

    /* Loop through the Limbs and save each one. */
    string limbFormSlugString;

    for (Limb& limb : map.getRoamingLimbs()) {
        limbFormSlugString = limb.getForm().slug;

        sqlite3_bind_text(limbStatement, 1, limbFormSlugString.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(limbStatement, 2, mapSlug, -1, SQLITE_STATIC);
        sqlite3_bind_int(limbStatement, 3, limb.getPosition().x);
        sqlite3_bind_int(limbStatement, 4, limb.getPosition().y);

        if (sqlite3_step(limbStatement) == SQLITE_DONE) {
            /* Get the ID of the saved item. */
            int limbID = static_cast<int>(sqlite3_last_insert_rowid(db));
            limb.setId(limbID);
        }
        else {
            cerr << "Insert failed for LIMB: " << sqlite3_errmsg(db) << endl;
            limbError = true;
            break;
        }

        sqlite3_reset(limbStatement);
    }

    /* Finalize the statement, commit the transaction. */
    sqlite3_finalize(limbStatement);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);

    if (limbError) { /* DELETE map and all blocks and all limbs. */ }


    /*
    * Now do a loop to save each Landmark.
    * 
    */

    /* Create statement for adding new Landmark object to the database. */
    const char* insertLandmarkSQL = "INSERT INTO landmark (map_slug, landmark_type, slug, position_x, position_y) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* landmarkStatement;

    /* Prepare the statement before starting the loop. */
    returnCode = sqlite3_prepare_v2(db, insertLandmarkSQL, -1, &landmarkStatement, nullptr);
    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare LANDMARK insert statement: " << sqlite3_errmsg(db) << endl;
        return false;
    }


    /* Begin the transaction. */
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    bool landmarkError = false;

    for (Landmark& landmark : map.getLandmarks()) {

        int landmarkType = landmark.getType();
        string slug = landmark.getSlug();

        /* Bind the data and execute the statement. */
        sqlite3_bind_text(landmarkStatement, 1, mapSlug, -1, SQLITE_STATIC);
        sqlite3_bind_int(landmarkStatement, 2, landmarkType);
        sqlite3_bind_text(landmarkStatement, 3, slug.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(landmarkStatement, 4, landmark.getPosition().x);
        sqlite3_bind_int(landmarkStatement, 5, landmark.getPosition().y);

        if (sqlite3_step(landmarkStatement) == SQLITE_DONE) {
            /* Get the ID of the saved item. */
            int landmarkID = static_cast<int>(sqlite3_last_insert_rowid(db));
            landmark.setId(landmarkID);
        }
        else {
            cerr << "Insert failed for LANDMARK: " << sqlite3_errmsg(db) << endl;
            landmarkError = true;
            break;
        }

        sqlite3_reset(landmarkStatement);
    }

    /* Finalize the statement, commit the transaction. */
    sqlite3_finalize(landmarkStatement);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);

    if (!landmarkError) { success = true; }
    else { /* DELETE map and all blocks and all limbs. */ }



    /* STILL TO COME: Characters. */



    /* Close the database. */
    sqlite3_close(db);
    return success;
}

export bool mapObjectExists(string mapSlug) {
    /*
    * 1. Make an SQL query string to check for map object with the slug from the parameters.
    * 2. open DB, execute query, get true or false into a variable, close DB.
    * 3. Return true or false.
    */

    int count = 0;

    /* Open database. */
    sqlite3* db;
    char* errMsg = nullptr;
    int dbFailed = sqlite3_open(dbPath(), &db);
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    /* Create statement template for querying the count. */
    const char* queryCountSQL = "SELECT COUNT(*) FROM map WHERE slug = ?;";
    sqlite3_stmt* statement;
    int returnCode = sqlite3_prepare_v2(db, queryCountSQL, -1, &statement, nullptr);

    if (returnCode != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    /* Bind the slug value. */
    sqlite3_bind_text(statement, 1, mapSlug.c_str(), -1, SQLITE_STATIC);

    /* Execute binded statement. */
    if (sqlite3_step(statement) == SQLITE_ROW) {
        count = sqlite3_column_int(statement, 0); }

    /* Finalize statement and close DB. */
    sqlite3_finalize(statement);
    sqlite3_close(db);

    return count > 0;
}


export Character loadPlayerCharacter(int characterID) {
    string name;
    string mapSlug;
    int anchorLimbID; /* Get from the limbs. */
    int battleID; /* For later, when battles actually exist. */
    bool isPlayer = true;
    Character character = Character(CharacterType::None);

    /* Open database. */
    sqlite3* db;
    char* errMsg = nullptr;
    int dbFailed = sqlite3_open(dbPath(), &db);
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return character;
    }
    

    /* GET THE CHARACTER OBJECT. */

    /* Create statement template for getting the character. */
    const char* queryMapSQL = "SELECT * FROM character WHERE id = ?;";
    sqlite3_stmt* characterStatement;
    int returnCode = sqlite3_prepare_v2(db, queryMapSQL, -1, &characterStatement, nullptr);

    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare character retrieval statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return character;
    }

    /* Bind the id value. */
    sqlite3_bind_int(characterStatement, 1, characterID);

    /* Execute binded statement. */
    if (sqlite3_step(characterStatement) != SQLITE_ROW) {
        cerr << "Failed to retrieve MAP: " << sqlite3_errmsg(db) << endl;
        return character;
    }

    name = stringFromUnsignedChar(sqlite3_column_text(characterStatement, 1));
    anchorLimbID = sqlite3_column_int(characterStatement, 2);
    mapSlug = stringFromUnsignedChar(sqlite3_column_text(characterStatement, 4));
    battleID = sqlite3_column_int(characterStatement, 5);

    /* Finalize statement. */
    sqlite3_finalize(characterStatement);


    /* GET THE LIMBS. */

    /* Create statement template for querying Map objects with this slug. */
    const char* queryLimbsSQL = "SELECT * FROM limb WHERE character_id = ?;";
    sqlite3_stmt* queryLimbsStatement;
    returnCode = sqlite3_prepare_v2(db, queryLimbsSQL, -1, &queryLimbsStatement, nullptr);

    if (returnCode != SQLITE_OK) {
        std::cerr << "Failed to prepare limbs retrieval statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return character;
    }

    /* Bind the slug value. */
    sqlite3_bind_int(queryLimbsStatement, 1, characterID);

    /* Execute and iterate through results. */
    while ((returnCode = sqlite3_step(queryLimbsStatement)) == SQLITE_ROW) {

        int limbID = sqlite3_column_int(queryLimbsStatement, 0);
        LimbForm limbForm = getLimbForm(stringFromUnsignedChar(sqlite3_column_text(queryLimbsStatement, 1)));
        string mapSlug = stringFromUnsignedChar(sqlite3_column_text(queryLimbsStatement, 3));
        int hpMod = sqlite3_column_int(queryLimbsStatement, 4);
        int strengthMod = sqlite3_column_int(queryLimbsStatement, 5);
        int speedMod = sqlite3_column_int(queryLimbsStatement, 6);
        int intelligenceMod = sqlite3_column_int(queryLimbsStatement, 7);
        int posX = sqlite3_column_int(queryLimbsStatement, 8);
        int posY = sqlite3_column_int(queryLimbsStatement, 9);
        int rotationAngle = sqlite3_column_int(queryLimbsStatement, 10);
        bool isAnchor = sqlite3_column_int(queryLimbsStatement, 11) == 1;
        bool isFlipped = sqlite3_column_int(queryLimbsStatement, 12) == 1;
        string limbName = stringFromUnsignedChar(sqlite3_column_text(queryLimbsStatement, 13));

        Point position = Point(posX, posY);

        Limb limb = Limb(sqlite3_column_int(queryLimbsStatement, 0),
            getLimbForm(stringFromUnsignedChar(sqlite3_column_text(queryLimbsStatement, 1))),
            position
        );

        limb.setName(limbName);
        limb.modifyHP(hpMod);
        limb.modifyStrength(strengthMod);
        limb.modifySpeed(speedMod);
        limb.modifyIntelligence(intelligenceMod);
        limb.rotate(rotationAngle);
        limb.setFlipped(isFlipped);
        limb.setAnchor(isAnchor);
        limb.setMapSlug(mapSlug);
        limb.setCharacterId(characterID);
        limb.setId(limbID);

        character.addLimb(limb);
    }

    if (returnCode != SQLITE_DONE) {
        cerr << "Execution failed: " << sqlite3_errmsg(db) << endl;
        return character;
    }

    /* Finalize prepared statement. */
    sqlite3_finalize(queryLimbsStatement);

    character = Character(CharacterType::Player);
    character.setName(name);
    character.setId(characterID);
    character.setAnchorLimbId(anchorLimbID);

    sqlite3_close(db);
    return character;
}



export Map loadMap(string mapSlug) {
    Map map;
    MapForm mapForm = getMapFormFromSlug(mapSlug);
    vector<Limb> roamingLimbs;
    vector<vector<Block>> rows(mapForm.blocksHeight);
    GameState& gameState = GameState::getInstance();

    /* Give each vector of blocks a size. */
    for (vector<Block>& vecOfBlocks : rows) {
        vecOfBlocks = vector<Block>(mapForm.blocksWidth);
    }

    /* Open database. */
    sqlite3* db;
    char* errMsg = nullptr;
    int dbFailed = sqlite3_open(dbPath(), &db);
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return map;
    }


    /* GET THE MAP OBJECT. */

    /* Create statement template for querying Map objects with this slug. */
    const char* queryMapSQL = "SELECT * FROM map WHERE slug = ?;";
    sqlite3_stmt* statement;
    int returnCode = sqlite3_prepare_v2(db, queryMapSQL, -1, &statement, nullptr);

    if (returnCode != SQLITE_OK) {
        cerr << "Failed to prepare map id retrieval statement: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return map;
    }

    /* Bind the slug value. */
    sqlite3_bind_text(statement, 1, mapSlug.c_str(), -1, SQLITE_STATIC);

    /* Execute binded statement. */
    if (sqlite3_step(statement) != SQLITE_ROW) {
        cerr << "Failed to retrieve MAP: " << sqlite3_errmsg(db) << endl;
        return map;
    }
    
    Point characterPosition = Point(
        sqlite3_column_int(statement, 1),
        sqlite3_column_int(statement, 2)
    );

    /* Finalize map ID retrieval statement. */
    sqlite3_finalize(statement);


    /* GET THE BLOCKS FROM THE DB. */

    /* Create statement template for querying Map objects with this slug. */
    const char* queryBlocksSQL = "SELECT * FROM block WHERE map_slug = ?;";
    sqlite3_stmt* blocksStatement;
    returnCode = sqlite3_prepare_v2(db, queryBlocksSQL, -1, &blocksStatement, nullptr);

    if (returnCode != SQLITE_OK) {
        std::cerr << "Failed to prepare blocks retrieval statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return map;
    }

    /* Bind the slug value. */
    sqlite3_bind_text(blocksStatement, 1, mapSlug.c_str(), -1, SQLITE_STATIC);

    /* Execute and iterate through results. */
    while ((returnCode = sqlite3_step(blocksStatement)) == SQLITE_ROW) {
        /* Position is not saved in the block. They dictate where in the rows and row the block is set. */
        int positionX = sqlite3_column_int(blocksStatement, 2);
        int positionY = sqlite3_column_int(blocksStatement, 3);

        int id = sqlite3_column_int(blocksStatement, 0);
        bool isFloor = sqlite3_column_int(blocksStatement, 4) == 1;
        bool isPath = sqlite3_column_int(blocksStatement, 5) == 1;
        bool isLooted = sqlite3_column_int(blocksStatement, 6) == 1;

        if (positionY < rows.size() && positionX < rows[positionY].size()) {
            rows[positionY][positionX] = Block(id, isFloor, isPath, isLooted);
        }
        else {
            cout << "Saved position out of bounds of map." << endl;
            break;
        }
    }

    if (returnCode != SQLITE_DONE) {
        cerr << "Execution failed: " << sqlite3_errmsg(db) << endl;
    }

    /* Finalize prepared statement. */
    sqlite3_finalize(blocksStatement);

    /* The Block objects are populated. Time to get the Limbs. */


    /* GET THE LIMBS FROM THE DB. */

    /* Create statement template for querying Map objects with this slug. */
    const char* queryLimbsSQL = "SELECT id, form_slug, position_x, position_y, character_id FROM limb WHERE map_slug = ? AND character_id < 1;";
    sqlite3_stmt* queryLimbsStatement;
    returnCode = sqlite3_prepare_v2(db, queryLimbsSQL, -1, &queryLimbsStatement, nullptr);

    if (returnCode != SQLITE_OK) {
        std::cerr << "Failed to prepare blocks retrieval statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return map;
    }

    /* Bind the slug value. */
    sqlite3_bind_text(queryLimbsStatement, 1, mapSlug.c_str(), -1, SQLITE_STATIC);

    /* Execute and iterate through results. */
    while ((returnCode = sqlite3_step(queryLimbsStatement)) == SQLITE_ROW) {
        cout << "Limb ID: " << sqlite3_column_int(queryLimbsStatement, 0) << "\n";
        roamingLimbs.emplace_back(
            sqlite3_column_int(queryLimbsStatement, 0),
            getLimbForm(stringFromUnsignedChar(sqlite3_column_text(queryLimbsStatement, 1))),
            Point(
                sqlite3_column_int(queryLimbsStatement, 2),
                sqlite3_column_int(queryLimbsStatement, 3)
            )
        );
    }

    if (returnCode != SQLITE_DONE) {
        cerr << "Execution failed: " << sqlite3_errmsg(db) << endl;
        return map;
    }

    /* Finalize prepared statement. */
    sqlite3_finalize(queryLimbsStatement);



    /* Get the LANDMARK objects. */

    /* Create statement template for querying Map objects with this slug. */
    const char* queryLandmarksSQL = "SELECT id, landmark_type, slug, position_x, position_y FROM landmark WHERE map_slug = ?;";
    sqlite3_stmt* queryLandmarksStatement;
    returnCode = sqlite3_prepare_v2(db, queryLandmarksSQL, -1, &queryLandmarksStatement, nullptr);

    if (returnCode != SQLITE_OK) {
        std::cerr << "Failed to prepare landmarks retrieval statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return map;
    }

    /* Bind the slug value. */
    sqlite3_bind_text(queryLandmarksStatement, 1, mapSlug.c_str(), -1, SQLITE_STATIC);

    vector<Landmark> landmarks;

    /* Execute and iterate through results. */
    while ((returnCode = sqlite3_step(queryLandmarksStatement)) == SQLITE_ROW) {

        int landmarkID = sqlite3_column_int(queryLandmarksStatement, 0);
        int landmarkTypeInt = sqlite3_column_int(queryLandmarksStatement, 1);
        LandmarkType landmarkType = isValidLandmarkType(landmarkTypeInt) ? static_cast<LandmarkType>(landmarkTypeInt) : Entrance;
        string landmarkSlug = stringFromUnsignedChar(sqlite3_column_text(queryLandmarksStatement, 2));
        int positionX = sqlite3_column_int(queryLandmarksStatement, 3);
        int positionY = sqlite3_column_int(queryLandmarksStatement, 4);

        Point position = Point(positionX, positionY);

        /*
        * WHEN we start making SHRINE landmarks, we will need to use more of this data and to call a builder
        * or else call a FORM which will get the TEXTURE.
        * 
        * FOR NOW we will just check for entrance and exit, and add them.
        */

        if (landmarkType == Entrance) {
            landmarks.emplace_back(getEntranceLandmark(position));
        } else if (landmarkType == Exit) {
            landmarks.emplace_back(getExitLandmark(position)); }
    }

    if (returnCode != SQLITE_DONE) {
        cerr << "Execution failed (retrieving landmarks): " << sqlite3_errmsg(db) << endl;
        return map;
    }

    /* Finalize prepared statement. */
    sqlite3_finalize(queryLandmarksStatement);


    /* Close DB. */
    sqlite3_close(db);

    /* BUILD THE MAP. */
    map = Map(mapForm, roamingLimbs, rows, characterPosition);
    map.setLandmarks(landmarks);

    return map;
}

export int createPlayerCharacterOrGetID() {

    /*
    * First check if player character already exists.
    * If it does, get the ID and return the ID.
    * If it does not exist, create the player character and return the ID.
    */

    int count = 0;
    int playerID = -1;

    /* Open database. */
    sqlite3* db;
    char* errMsg = nullptr;
    int dbFailed = sqlite3_open(dbPath(), &db);
    if (dbFailed != 0) {
        cerr << "Error opening DB: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    /* Create statement template for querying the count. */
    const char* queryCountSQL = "SELECT COUNT(*) FROM character WHERE is_player = 1;";
    sqlite3_stmt* statement;
    int returnCode = sqlite3_prepare_v2(db, queryCountSQL, -1, &statement, nullptr);

    if (returnCode != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    /* Execute statement. */
    if (sqlite3_step(statement) == SQLITE_ROW) {
        count = sqlite3_column_int(statement, 0);
    }

    /* Finalize statement. */
    sqlite3_finalize(statement);

    if (count > 0) {
        /* Player character exists. Get the ID. */

        /* Create statement template for querying the id. */
        const char* queryIDSQL = "SELECT id FROM character WHERE is_player = 1;";
        sqlite3_stmt* idStatement;
        returnCode = sqlite3_prepare_v2(db, queryIDSQL, -1, &idStatement, nullptr);

        if (returnCode != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_close(db);
            return false;
        }

        /* Execute statement. */
        if (sqlite3_step(idStatement) == SQLITE_ROW) {
            playerID = sqlite3_column_int(idStatement, 0);
            sqlite3_finalize(idStatement);
            sqlite3_close(db);
            cout << "Player ID retrieved: " << playerID << "\n";
            return playerID;
        }

        /* Finalize statement. */
        sqlite3_finalize(idStatement);
    }


    /* Player character does not exist. Create it. */

    const char* newCharacterSQL = "INSERT INTO character (name, is_player) VALUES (?, ?);";
    sqlite3_stmt* newCharStatement;
    returnCode = sqlite3_prepare_v2(db, newCharacterSQL, -1, &newCharStatement, nullptr);

    /* Bind values. */
    const char* name = "Player";
    int isPlayerBoolInt = 1;
    sqlite3_bind_text(newCharStatement, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(newCharStatement, 2, isPlayerBoolInt);

    /* Execute the statement. */
    returnCode = sqlite3_step(newCharStatement);
    if (returnCode != SQLITE_DONE) { cerr << "Insert Player Character failed: " << sqlite3_errmsg(db) << endl; }
    else {
        /* Get the ID of the saved item. */
        playerID = static_cast<int>(sqlite3_last_insert_rowid(db));
    }

    /* Close DB. */
    sqlite3_finalize(newCharStatement);
    sqlite3_close(db);

    return playerID;
}