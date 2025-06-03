async function createUser() {
    const name = document.getElementById('name').value;
    const email = document.getElementById('email').value;
    const password = document.getElementById('password').value;
    
    try {
        const response = await fetch('/api/users', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({name, email, password})
        });
        
        const result = await response.text();
        document.getElementById('result').innerHTML = `Response: ${response.status}\n${result}`;
    } catch (error) {
        document.getElementById('result').innerHTML = `Error: ${error.message}`;
    }
}

async function getUsers() {
    try {
        const response = await fetch('/api/users');
        const result = await response.text();
        document.getElementById('result').innerHTML = `Response: ${response.status}\n${result}`;
    } catch (error) {
        document.getElementById('result').innerHTML = `Error: ${error.message}`;
    }
}

// Add some interactivity
document.addEventListener('DOMContentLoaded', function() {
    console.log('C HTTP Server static file serving is working!');
});