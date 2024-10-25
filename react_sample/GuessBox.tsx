/* GuessBox handles API calls to the database when:
 *      1: Entering a guess item --> confirm which fields are correct, if the item exists
 *      2: Optionally enable autocomplete pop-up under text box
 */

import {useState, useEffect} from 'react';


function isAlpha(str:string) {
    // regex search to confirm only alpha characters (plus space)
    return /^[a-zA-Z ]+$/.test(str);
}

class GuessEntity {
    ID: string;
    profession: string;
    season: string;
    sellPrice: string;

    constructor(id: string, prof: string, seas: string, sell: string) {
        this.ID = id;
        this.profession = prof;
        this.season = seas;
        this.sellPrice = sell;
    }

    toString() {
        return `${this.ID}: ${this.profession}, ${this.season}, ${this.sellPrice}G`;
    }

    compare(target: GuessEntity) {
        // compare this guess entity to the answer item
        // returnTuple: [boolean, boolean, boolean, string];
        let priceChar = '';
        const selfInt = parseInt(this.sellPrice);
        const targetInt = parseInt(target.sellPrice);
        
        if (selfInt > targetInt) {
            priceChar = "greater";  // our guess was too high ==> guess item is less than our guess
            this.sellPrice = "< " + this.sellPrice;
        }
        else if (selfInt < targetInt) {
            priceChar = "less";
            this.sellPrice = "> " + this.sellPrice;
        }
        else {
            priceChar = "true";
        }

        // determine season string, since it may be multi-season
        let seasonStr = "false";

        if (this.season === target.season){
            seasonStr = "true";
        }
        
        // not a complete match ==> find partial match (if target / self has multiple parts to answer)
        else if (this.season.includes(",") || target.season.includes(",")){
            const selfTokens = this.season.split(",");
            const targetTokens = target.season.split(",");
            for (let i = 0; i < selfTokens.length; i++){
                for (let j = 0; j < targetTokens.length; j++){
                    if (selfTokens[i] === targetTokens[j]){
                        seasonStr = "partial";
                    }
                }
            }
        }

        let ret_vals: Array<string | boolean | null> = [this.ID, this.profession, this.season, this.sellPrice.toString() + "g",
            this.ID == target.ID, this.profession == target.profession, seasonStr, priceChar];
        return ret_vals;
    }
}



async function getAPICall(inName: string) {
    const loweredInput = inName.toLowerCase();
    if ( !(isAlpha(loweredInput)) ) {
        console.log("ERROR: Only input alphabet characters!");
        return
    }
    
    // create header for GET request
    const headers = new Headers();
    headers.set('Content-Type', 'application/json');
    headers.set('Accept', 'application/json');

    // create url and add parameters for API search 
    let getURL = new URL("https://pouq9pcpxk.execute-api.us-west-2.amazonaws.com/Dev-stage");
    getURL.searchParams.append('ID', loweredInput);

    // send a GET Request to the API  
    const requestOptions: RequestInfo = new Request(getURL, {
        method: "GET",
        headers: headers,
        redirect: 'follow'
    })
    console.log("sending request:", getURL.toString())

    // wait for response
    const response = await fetch(requestOptions);
    const resultText = await response.text();
    const parsed_res = JSON.parse(resultText);
    const parsed_body = JSON.parse(parsed_res.body);
    const inner = parsed_body.Item;
    return inner;
}

function GuessBox() {
    // non-complete portion of the GuessBox component -- For showcasing a use case of the getAPICall function
    //  
    const [guess, setGuess] = useState("");
    const [usedItems, setUsedItems] = useState<string[]>([]);
    const [answer, setAnswer] = useState(new GuessEntity("null", "null", "null", "null"))
    const [prevGuesses, setPrevGuesses] = useState(createInitialGuesses); 
    const [turn, setTurn] = useState(0);

    const handleOnSelect = (item:AutocompleteItem) => {
        // unlike submitHandler which runs after the "guess" state is updated,
        // this call handles requests to render a new item without needing to guarantee the "guess" state is updated
        console.log("Select:", item)
        setGuess(item.name);
        getAPICall(item.name)
            .then(response => {
                console.log("respond", response);
                let guessedItem = new GuessEntity(response.ID, response.profession, response.season, response.sellPrice.toString());
                let correctnessArray = guessedItem.compare(answer);
                    
                if (usedItems.includes(guessedItem.ID)) {
                    alert("already guessed, skip");
                }                    
                else {  // otherwise, we have a new guess

                    setPrevGuesses([{guess_num: turn, values: correctnessArray}, ...prevGuesses]);

                    // update prevGuesses with rebuilt array
                    
                    setUsedItems([...usedItems, guessedItem.ID]);
                    console.log("previous guess items: ", usedItems);
                    
                    // correctnessArray[1] stores whether or not it's a match
                    if (correctnessArray[4] == true) {  // win condition
                        setMatchBool(true);
                    }

                    else if (turn === 25) {  // loss condition
                        alert("game lost.");
                    }
                    setTurn(i => i + 1);  
                }
            });
    }


}