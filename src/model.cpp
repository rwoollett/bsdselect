#include "storage/storage.h"
#include <string>

using namespace std;

struct MovieAttrs
{
  struct Table
  {
    string Movies{"Movies"};
  } T;
  struct Fields
  {
    string ID{"M_ID"};
    string Movie{"Movie"};
  } F;
};

//-------------------------------------------------------
// Main
//-------------------------------------------------------

int main(int argc, char **argv)
{
  string userName{"TEST"};
  string userPassword{"pass"};
  string role{"GUEST"};
  string driver{"Sqlite"}, server{"localhost"}, dbname{"test.db"}, charset{"ISO8859_1"};
  DatabaseConnection dbConnect{driver, server, dbname, charset};

  vector<string> movies{};
  Schema schemaMovie{{{"M_ID", FieldType::PK_INT},
                      {"Movie", FieldType::STRING}}};
  MovieAttrs MA;
  Storage::DBStorage dbStorage(dbConnect);

  Model moviesModel = dbStorage.model(MA.T.Movies, schemaMovie);//, CreateKind::DROP_CREATE);

  // dbStorage.beginTransaction();
  // for (int i = 0; i < 5000; i++) {
  //   auto e = moviesModel.entity();
  //   e->set(MA.F.ID, std::to_string(i));
  //   e->set(MA.F.Movie, ("New movies" + std::to_string(i)));
  //   e->save();
  // }
  // dbStorage.commit();

  auto list = moviesModel.find({ {MA.F.ID, "1234"}});

  for (auto& entity: list) {
    std::cout << "Movie: " << std::get<0>(entity->get(MA.F.ID)) << " " << std::get<0>(entity->get(MA.F.Movie)) << "\n ";
  }

  return 0;
};
