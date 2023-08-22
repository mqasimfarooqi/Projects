const $RefParser = require("@apidevtools/json-schema-ref-parser");
const fs = require('fs');

async function start() {
    let schema = await $RefParser.bundle("test-schemas/rule-engine-schema.json");
    fs.writeFile("bundled-schema.json",
    JSON.stringify(schema, null, 2), function(err) {
        if(err) {
            return console.log(err);
        }
        console.log("The file was saved!");
    }); 
}
start()