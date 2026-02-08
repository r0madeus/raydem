#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables
const float samples = 5.0;          // Pixels per axis; higher = bigger glow, worse performance
const float quality = 2.5;          // Lower = smaller glow, better quality

void main()
{
    vec4 sum = vec4(0);
    vec2 size = textureSize(texture0, 0);
    int diff = (int(samples) - 1) / 2;
    
    for (int x = -diff; x <= diff; x++)
    {
        for (int y = -diff; y <= diff; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * quality;
            sum += texture(texture0, (fragTexCoord * size + offset) / size);
        }
    }
    
    finalColor = ((sum / (samples * samples)) + texture(texture0, fragTexCoord)) * colDiffuse;
}
