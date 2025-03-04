import pandas as pd

# Read the dataset
df = pd.read_csv('doctorwho1.csv')

# Create a mapping of characters to integers
char_to_int = {char: idx for idx, char in enumerate(sorted(set(df['Source'].unique()) | set(df['Target'].unique())))}

# Create a DataFrame for character-to-integer mapping
char_int_df = pd.DataFrame({'Character': list(char_to_int.keys()), 'Integer': list(char_to_int.values())})

# Map characters to integers in the DataFrame
df['Source'] = df['Source'].map(char_to_int)
df['Target'] = df['Target'].map(char_to_int)

# Write the mappings to a text file
with open('doctorwho_mapping.txt', 'w') as f:
    for idx, row in df.iterrows():
        f.write(f"{row['Source']}\t{row['Target']}\n")

# Write the character-to-integer mapping to a CSV file
char_int_df.to_csv('character_integer_mapping.csv', index=False)
