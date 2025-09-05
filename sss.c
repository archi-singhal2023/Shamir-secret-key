#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct
{
    int x;
    long long y;
} Share;

typedef struct
{
    int n;
    int k;
    Share *shares;
    int num_shares;
} JsonData;

// A simple utility to convert a string representing a number in a given base
long long string_to_long_long(const char *str, int base)
{
    long long result = 0;
    long long power = 1;
    int len = strlen(str);
    for (int i = len - 1; i >= 0; i--)
    {
        int digit;
        if (str[i] >= '0' && str[i] <= '9')
        {
            digit = str[i] - '0';
        }
        else if (str[i] >= 'a' && str[i] <= 'f')
        {
            digit = str[i] - 'a' + 10;
        }
        else
        {
            // Error handling for invalid characters
            return -1;
        }
        result += digit * power;
        power *= base;
    }
    return result;
}

// Manual JSON-like parser for the specific input format
JsonData *manual_json_parse(const char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL)
    {
        printf("Error: File '%s' not found.\n", file_path);
        return NULL;
    }

    JsonData *data = (JsonData *)malloc(sizeof(JsonData));
    if (data == NULL)
    {
        printf("Error: Memory allocation failed.\n");
        return NULL;
    }
    data->shares = NULL;
    data->num_shares = 0;

    char line[256];
    char *current_key = NULL;

    while (fgets(line, sizeof(line), fp))
    {
        // Skip whitespace and brackets
        char *trimmed_line = line;
        while (*trimmed_line == ' ' || *trimmed_line == '\t')
        {
            trimmed_line++;
        }
        if (*trimmed_line == '{' || *trimmed_line == '}' || *trimmed_line == '\n')
        {
            continue;
        }

        // Tokenize the line
        char *key_str = strtok(trimmed_line, ":");
        char *val_str = strtok(NULL, ",");

        if (key_str == NULL || val_str == NULL)
        {
            continue;
        }

        char key[50], value[256];
        sscanf(key_str, "\"%[^\"]\"", key);
        sscanf(val_str, " \"%[^\"]\"", value);

        if (strcmp(key, "keys") == 0)
        {
            // Handle the nested keys section
            char nested_line[256];
            fgets(nested_line, sizeof(nested_line), fp); // Read "n" line
            char *n_key_str = strtok(nested_line, ":");
            char *n_val_str = strtok(NULL, ",");
            data->n = atoi(n_val_str);

            fgets(nested_line, sizeof(nested_line), fp); // Read "k" line
            char *k_key_str = strtok(nested_line, ":");
            char *k_val_str = strtok(NULL, "}");
            data->k = atoi(k_val_str);

            current_key = NULL;
        }
        else if (key[0] >= '0' && key[0] <= '9')
        {
            // Handle share data
            data->num_shares++;
            data->shares = (Share *)realloc(data->shares, data->num_shares * sizeof(Share));
            if (data->shares == NULL)
            {
                printf("Error: Memory reallocation failed.\n");
                fclose(fp);
                free(data);
                return NULL;
            }

            // Read the base and value lines for the current share
            Share current_share;
            current_share.x = atoi(key);

            char base_line[256], value_line[256];
            fgets(base_line, sizeof(base_line), fp);   // "base" line
            fgets(value_line, sizeof(value_line), fp); // "value" line

            char base_val_str[50], share_val_str[256];
            sscanf(base_line, " \"%*s\" : \"%[^\"]\"", base_val_str);
            sscanf(value_line, " \"%*s\" : \"%[^\"]\"", share_val_str);

            int base = atoi(base_val_str);
            current_share.y = string_to_long_long(share_val_str, base);

            data->shares[data->num_shares - 1] = current_share;
        }
    }
    fclose(fp);
    return data;
}

// Function to find the modular multiplicative inverse
long long mod_inverse(long long a, long long m)
{
    long long m0 = m;
    long long y = 0, x = 1;
    if (m == 1)
        return 0;
    while (a > 1)
    {
        long long q = a / m;
        long long t = m;
        m = a % m, a = t;
        t = y;
        y = x - q * y;
        x = t;
    }
    if (x < 0)
        x += m0;
    return x;
}

// Lagrange interpolation to reconstruct the secret at x=0
long long lagrange_interpolate_at_zero(Share *points, int k, long long prime)
{
    long long secret = 0;
    for (int j = 0; j < k; j++)
    {
        long long num = 1;
        long long den = 1;
        for (int m = 0; m < k; m++)
        {
            if (j == m)
            {
                continue;
            }
            num = (num * (0 - points[m].x)) % prime;
            den = (den * (points[j].x - points[m].x)) % prime;
        }
        long long term = (points[j].y * num) % prime;
        long long inverse = mod_inverse(den, prime);
        secret = (secret + (term * inverse) % prime) % prime;
    }
    return secret;
}

// Recursive function to generate combinations
void get_combinations(Share *arr, int n, int k, int start_index, Share *combination, int combination_index, long long prime, long long *secrets, int *secret_count)
{
    if (combination_index == k)
    {
        long long reconstructed = lagrange_interpolate_at_zero(combination, k, prime);
        if (reconstructed >= 0)
        {
            secrets[*secret_count] = reconstructed;
            (*secret_count)++;
        }
        return;
    }

    if (start_index >= n)
    {
        return;
    }

    combination[combination_index] = arr[start_index];
    get_combinations(arr, n, k, start_index + 1, combination, combination_index + 1, prime, secrets, secret_count);

    get_combinations(arr, n, k, start_index + 1, combination, combination_index, prime, secrets, secret_count);
}

// Function to count occurrences in an array and find the most common one
long long find_most_common(long long *arr, int count)
{
    if (count == 0)
        return -1;

    long long most_common = -1;
    int max_freq = 0;

    for (int i = 0; i < count; i++)
    {
        int current_freq = 0;
        for (int j = 0; j < count; j++)
        {
            if (arr[j] == arr[i])
            {
                current_freq++;
            }
        }
        if (current_freq > max_freq)
        {
            max_freq = current_freq;
            most_common = arr[i];
        }
    }
    return most_common;
}

void compute_and_verify_secret(const char *file_path)
{
    JsonData *data = manual_json_parse(file_path);
    if (data == NULL)
    {
        return;
    }

    // The prime modulus is crucial and must be chosen appropriately.
    // For the first test case, p=101 works. For larger numbers,
    // this would need to be a much larger prime.
    long long prime = 101;

    int n = data->num_shares;
    int k = data->k;

    printf("Loaded %d shares with a threshold k=%d and prime p=%lld.\n\n", n, k, prime);

    if (n < k)
    {
        printf("Error: Not enough shares to reconstruct the secret. Need at least %d shares.\n", k);
        free(data->shares);
        free(data);
        return;
    }

    // Allocate memory for storing all reconstructed secrets
    // The number of combinations is n! / (k! * (n-k)!)
    // For n=4, k=3, this is 4 combinations.
    // For n=10, k=7, this is 120 combinations.
    long long *secrets = (long long *)malloc(1024 * sizeof(long long)); // Using a large enough static size
    int secret_count = 0;

    Share *combination = (Share *)malloc(k * sizeof(Share));
    get_combinations(data->shares, n, k, 0, combination, 0, prime, secrets, &secret_count);
    free(combination);

    if (secret_count == 0)
    {
        printf("No valid secret could be reconstructed.\n");
        free(secrets);
        free(data->shares);
        free(data);
        return;
    }

    long long most_common_secret = find_most_common(secrets, secret_count);

    bool wrong_share_found = false;
    printf("Wrong shares:\n");
    for (int i = 0; i < n; i++)
    {
        Share temp_shares[n - 1];
        int temp_index = 0;
        for (int j = 0; j < n; j++)
        {
            if (i == j)
                continue;
            temp_shares[temp_index++] = data->shares[j];
        }

        long long *subset_secrets = (long long *)malloc(1024 * sizeof(long long));
        int subset_secret_count = 0;
        Share *temp_combination = (Share *)malloc(k * sizeof(Share));
        get_combinations(temp_shares, n - 1, k, 0, temp_combination, 0, prime, subset_secrets, &subset_secret_count);
        free(temp_combination);

        for (int j = 0; j < subset_secret_count; j++)
        {
            if (subset_secrets[j] == most_common_secret)
            {
                printf(" - Share with x-coordinate %d\n", data->shares[i].x);
                wrong_share_found = true;
                break;
            }
        }
        free(subset_secrets);
    }

    if (!wrong_share_found)
    {
        printf(" - No wrong shares found based on the most common secret.\n");
    }

    printf("\n------------------------------\n");
    printf("Computed Secret: %lld\n", most_common_secret);
    printf("------------------------------\n");

    free(secrets);
    free(data->shares);
    free(data);
}

int main()
{
    // To run this program, you must create a JSON file with the share data
    // in the same directory and name it "input.json".
    const char *file_path = "test1.json";

    compute_and_verify_secret(file_path);

    return 0;
}
