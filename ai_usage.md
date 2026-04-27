1. Tool Used
Google Gemini (AI Collaborator)

2. Prompts Given
I provided the AI with the project specification images and my current source code. My primary prompt was:

"Generate the AI-assisted filtering logic: specifically the parse_condition and match_condition functions based on my Report struct."

3. What Was Generated
A parsing system using sscanf with the %[^:] format to split user input by colons.


4. Changes I Made and Why
The initial AI-generated code separated the filtering from the list command. I integrated them so that list could optionally accept a filter string as a third argument.


5. What I Learned
I realized that you cannot simply delete a line in a binary file like you do in a text editor. In stead you must manually shift data or rewrite the file and use ftruncate to inform the OS the file has shrunk.

I gained a better understanding of how rwxrwxrwx bits translate into the numbers used in C code.