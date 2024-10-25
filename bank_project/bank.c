#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "parser.h"
#include "account.h"


account* accounts;
int num_accounts;
int successful_events = 0; 
int failed_events = 0;
int num_updates = 0;

int lookup_account(account* account_list, char* account){
    int ret = -1;
    for (int i = 0; i < num_accounts; i++){
        if (strcmp(account_list[i].account_number, account) == 0){
            ret = i;
        }
    }
    return ret;
}

void* process_transaction(void* arg){
    // function run by worker thread to handle transction request
    
    command_line tokens = *((command_line*) arg);

    int account = lookup_account(accounts, tokens.command_list[1]);
    if (account == -1){
        // printf("account does not exist!\n");
        return NULL;
    }
    // confirm password
    if (strcmp(tokens.command_list[2], accounts[account].password) != 0){
        // printf("wrong password!\n");
        return NULL;
    }
    
    // printf("account [%d]: %s -- %s\n", account, accounts[account].account_number, tokens.command_list[1]);
    if (strcmp(tokens.command_list[0],"T") == 0){
        // transfer
        int dst_acc = lookup_account(accounts, tokens.command_list[3]);
        if (dst_acc == -1){
            // printf("destination account does not exist!\n");
            return NULL;
        }
        double amount = atof(tokens.command_list[4]);
        // only add to account that called the transfer
        accounts[account].balance -= amount;
        accounts[account].transaction_tracter += amount;
        
        accounts[dst_acc].balance += amount;
    }
    
    if (strcmp(tokens.command_list[0],"C") == 0){
        // balance check 
        // printf("%d balance: %f\n", account, accounts[account].balance);
    }
    
    if (strcmp(tokens.command_list[0],"D") == 0){
        // deposit 
        double amount = atof(tokens.command_list[3]);
        accounts[account].balance += amount;
        accounts[account].transaction_tracter += amount;

    }
    
    if (strcmp(tokens.command_list[0],"W") == 0){
        // withdraw 
        double amount = atof(tokens.command_list[3]);
        accounts[account].balance -= amount;
        accounts[account].transaction_tracter += amount;
    }
    return NULL;
}

void* update_balance(void* arg){
    /*
    function run by dedicated bank thread to update each account balance based on reward rate & transacion tracker
    function returns the number of times it had to update each account
    */
    char in_text[50];
    sprintf(in_text, "output/output.txt");
    FILE* account_out = fopen(in_text, "w");
    
    
    for (int i = 0; i < num_accounts; i++){
        
        double reward = accounts[i].transaction_tracter * accounts[i].reward_rate;
        accounts[i].balance += reward; 

        fprintf(account_out, "%d balance: %.2f\n\n", i, accounts[i].balance);
    }
    fclose(account_out);
    num_updates ++;
     
    return &num_updates;
}


int main(int argc,  char* argv[]){

    // confirm proper usage
    if (argc != 2){
        printf("bank.c called incorrectly! Format: ./bank {input file}\n");
        exit(-1);
    }

    // open file
    FILE* bank_file;
    bank_file = fopen(argv[1], "r");
    if (bank_file == NULL){
        printf("Invalid File.\n");
        exit(-1);
    }
    
    size_t buf_size = 128;
    char* line_buf = malloc(buf_size);

    // get the number of accounts by pulling first line
    getline(&line_buf, &buf_size, bank_file);
    num_accounts = atoi(line_buf);
    // printf("num accounts: %d\n", num_accounts); 
    accounts = (account*) malloc( sizeof(account) * num_accounts);

    for (int i = 0; i < num_accounts; i++){
        // skip index {i} line
        getline(&line_buf, &buf_size, bank_file);
        account curr_account;
        
        // get account # (char*)
        getline(&line_buf, &buf_size, bank_file);
        strcpy(curr_account.account_number, strtok(line_buf, "\n"));
        
        // get pword (char*)
        getline(&line_buf, &buf_size, bank_file);
        strcpy(curr_account.password, strtok(line_buf, "\n"));
        
        // get initial balance (double)
        getline(&line_buf, &buf_size, bank_file);
        curr_account.balance = atof(line_buf);
        
        // get reward rate (double)
        getline(&line_buf, &buf_size, bank_file);
        curr_account.reward_rate = atof(line_buf);

        // init bonus info
        curr_account.transaction_tracter = 0;
        curr_account.ac_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

        accounts[i] = curr_account;
    }

    printf("Initial account information\n");
    for (int i = 0; i < num_accounts; i++){
        printf("Account %s:   Pass: %s   Init Balance: %.2f   Rate: %.3f\n", 
        accounts[i].account_number, accounts[i].password,
        accounts[i].balance, accounts[i].reward_rate);
    }
    printf("---------------------------------------------------------------\n");

    
    while (getline (&line_buf, &buf_size, bank_file) != -1){
        // parse rest of transactions
        command_line tokens = str_filler(line_buf, " ");
        // printf("current action: %s\n", tokens.command_list[0]);
        
        process_transaction(&tokens);
        free_command_line(&tokens);
    }

    // transactions completed: print out results

    update_balance(NULL);

    FILE* output;
    output = fopen("output/output.txt", "w");

    printf("Final balances:\n");
    for (int i = 0; i < num_accounts; i++){
        fprintf(output, "%d balance: %.2f\n", i, accounts[i].balance);
        printf("%d balance: %.2f\n", i, accounts[i].balance);
    }

    fclose(bank_file);
    fclose(output);
    free(line_buf);
    free(accounts);
}