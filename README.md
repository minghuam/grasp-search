### Compile original Graspit code
Download version 2.2 from [here](http://sourceforge.net/projects/graspit/) and follow the manual to install dependencies.

1. dlopen@GLIBC error: add `-ldl` to `LIBS` in `graspit-lib-LINUX.pro`.

2. Hoxfix 1 in `ivmgr.cpp`: change `SbList<SbName> extList` to `SbPList extList`.

3. Hotfix 2 in `database.cpp`:
 
    Original code:


        bool Table::Populate(QSqlQuery query) {
          PROF_TIMER_FUNC(TABLE_POPULATE);
          data_.clear();
          column_names_.clear();
          if (query.next()) {
            QSqlRecord first_record = query.record();
            num_columns_ = first_record.count();
            data_.resize(num_columns_, vector<QVariant>(query.numRowsAffected()));
            for (int i = 0; i < num_columns_; ++i) {
              column_names_.insert(make_pair(first_record.fieldName(i).toStdString(), i));
              //cerr << "Column name: " << first_record.fieldName(i).toStdString() << "\n";
            }
            do {
              for (int i = 0; i < num_columns_; ++i) 
                data_[i].push_back(query.value(i));
            } while(query.next());
            if (!data_.empty()) num_rows_ = data_[0].size();
            return true;
          }
          return false;
        }

    Modified code:

        bool Table::Populate(QSqlQuery query) {
          PROF_TIMER_FUNC(TABLE_POPULATE);
          data_.clear();
          column_names_.clear();
          if (query.next()) {
            QSqlRecord first_record = query.record();
            num_columns_ = first_record.count();
            //data_.resize(num_columns_, vector<QVariant>(query.numRowsAffected()));
            for (int i = 0; i < num_columns_; ++i) {
              column_names_.insert(make_pair(first_record.fieldName(i).toStdString(), i));
              //cerr << "Column name: " << first_record.fieldName(i).toStdString() << "\n";
              vector<QVariant> v;
              data_.push_back(v);
            }
            do {
              for (int i = 0; i < num_columns_; ++i) 
                data_[i].push_back(query.value(i));
            } while(query.next());
            if (!data_.empty()) num_rows_ = data_[0].size();
            return true;
          }
          return false;
        }


4. To save images, install Simage library: 
    
    `apt-get install libsimage-dev`


### Prepare database
1. Follow [here](http://grasping.cs.columbia.edu/) to download the grasp database file and shape models.

2. Create a database table `cgdb` and restore database: 
    
    `sudo -u postgres psql cgdb < path_to_cgdb-10.0.backup`
    
    Ignore `...$libdir/cube$...` errors and warnings for now.

3. Setup environtment variables: `GRASPIT` and `CGDB_MODEL_ROOT`.

![Car](https://github.com/minghuam/grasp-search/blob/master/doc/car.png)

![Airplane](https://github.com/minghuam/grasp-search/blob/master/doc/airplane.png)

![Horse](https://github.com/minghuam/grasp-search/blob/master/doc/horse.png)



