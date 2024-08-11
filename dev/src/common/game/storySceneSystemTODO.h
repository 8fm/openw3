

// Skoro nie mozemy zmienic za duzo to:
// Jedyna klasa widziana poza scenami powinien byc StorySceneController bez StoryScenePlayera
// Wyczyscic zaleznosci w dwie storny miedzy wszystkimi klasami w scenach czyli klasa A ma pointera na B i B na A
// interface zamiast CActor w scenach + super wazne wyjebac THandlery, nie sa potrzebne jak sie cos wyjebuje to jest to bug a nie 
// zamiatac pod dywan THandlami i if ( Get() )
