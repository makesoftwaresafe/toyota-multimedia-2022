use mydb;

// --- special command helpers
help;
db.help();
show dbs;
show collections;
show users;
show profile;

// --- basic shell JS operations
db;
db.auth('foo', 'bar');

db.things.help();
things = db.things;

bing = { N : "bing" };
bang = { N : "bang" };
boom = { N : "boom" };
things.find();

things.remove(bing);
things.find();

things.save(bing);
things.remove(bing);
things.find();

things.insert(bing);
things.insert(bang);
things.insert(boom);
things.ensureIndex( { N: 1 } );
things.remove(bang);
things.find();

things.update(bing, bang, false, false);
things.find();
things.update(bang, bing, true, false);
things.find();
things.insert(bang);
things.insert(bang);
// things.update(bang, bing, true, true);
things.find();

// --- cleanup
things.drop();

db.getSisterDB('test').getCollectionNames();


// --- tutorial
j = { name : "mongo" };
t = { x : 3 };
things.save(j);
things.save(t);
things.find();

for (var i = 1; i <= 20; i++) things.save({x : 4, j : i});
things.find();

var cursor = things.find();
while (cursor.hasNext()) printjson(cursor.next());

things.find().forEach(printjson);

var cursor = things.find();
printjson(cursor[4]);

var arr = things.find().toArray();
arr[5];

things.find({name:"mongo"}).forEach(printjson);

things.find({x:4}).forEach(printjson);

things.find({x:4}, {j:true}).forEach(printjson);

printjson(things.findOne({name:"mongo"}));

things.find().limit(3);

printjson;

// --- cleanup
things.drop();

exit;
