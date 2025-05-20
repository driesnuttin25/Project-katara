// Import Firebase modules
import { initializeApp } from 'https://www.gstatic.com/firebasejs/9.15.0/firebase-app.js';
import { getDatabase, ref, onValue } from 'https://www.gstatic.com/firebasejs/9.15.0/firebase-database.js';

// Firebase configuration
const firebaseConfig = {
    apiKey: "REMOVED_FOR_CHAT",
    authDomain: "project-katara.firebaseapp.com",
    databaseURL: "REMOVED_FOR_CHAT",
    projectId: "project-katara",
    storageBucket: "project-katara.firebasestorage.app",
    messagingSenderId: "7288655338",
    appId: "REMOVED_FOR_CHAT",
    measurementId: "G-J465YLGVQ9"
};

// Initialize Firebase and Database
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

document.addEventListener('DOMContentLoaded', () => {
        // Image Upload and Handling
        const activeSquare = document.querySelector('.plant-square.active');
        if (activeSquare) {
            activeSquare.addEventListener('click', () => {
                window.location.href = 'plant.html';
            });
        }
            // ***** INDEX PAGE SCRIPT *****
            if (document.getElementById('plant-grid')) {
                // Variables
                const plantGrid = document.getElementById('plant-grid');
                const addPlantModal = document.getElementById('add-plant-modal');
                const closeModalButton = document.getElementById('close-modal');
                const addPlantForm = document.getElementById('add-plant-form');
        
                // Load plants from localStorage
                let plants = JSON.parse(localStorage.getItem('plants')) || [];
        
                // Function to render the plant grid
                function renderPlantGrid() {
                    plantGrid.innerHTML = '';
                    for (let i = 0; i < 4; i++) {
                        if (plants[i]) {
                            const plantSquare = document.createElement('div');
                            plantSquare.classList.add('plant-square', 'active');
                            plantSquare.setAttribute('data-id', plants[i].id);
                
                            const img = document.createElement('img');
                
                            // Determine the image to display
                            if (plants[i].image) {
                                img.src = plants[i].image; // Custom image if provided
                            } else {
                                const plantType = plants[i].type.toLowerCase().replace(/\s+/g, '_');
                                const growthStage = plants[i].growthStage ? plants[i].growthStage.toLowerCase() : 'sprout';
                                img.src = `assets/grow_stages/${plantType}_${growthStage}.png`;
                            }
                
                            img.alt = 'Plant Image';
                
                            const plantName = document.createElement('p');
                            plantName.classList.add('plant-name');
                            plantName.textContent = plants[i].name;
                
                            plantSquare.appendChild(img);
                            plantSquare.appendChild(plantName);
                            plantGrid.appendChild(plantSquare);
                        } else {
                            const plantSquare = document.createElement('div');
                            plantSquare.classList.add('plant-square', 'inactive');
                            plantSquare.innerHTML = '<div class="plus-icon">+</div>';
                            plantGrid.appendChild(plantSquare);
                        }
                    }
                }
                

        
                // Initial render
                renderPlantGrid();
        
                // Event listener for adding new plant
                plantGrid.addEventListener('click', (e) => {
                    if (e.target.closest('.inactive')) {
                        addPlantModal.style.display = 'block';
                    } else if (e.target.closest('.active')) {
                        const plantId = e.target.closest('.active').getAttribute('data-id');
                        localStorage.setItem('currentPlantId', plantId);
                        window.location.href = 'plant.html';
                    }
                });
        
                // Close modal
                closeModalButton.addEventListener('click', () => {
                    addPlantModal.style.display = 'none';
                });
        
                // Handle form submission
                addPlantForm.addEventListener('submit', (e) => {
                    e.preventDefault();
                    if (plants.length >= 4) {
                        alert('You can only add up to 4 plants.');
                        return;
                    }
                    const plantType = document.getElementById('plant-type').value;
                    const plantName = document.getElementById('plant-name').value;
                    const wateringOption = document.getElementById('watering-option').value;
        
                    const newPlant = {
                        id: Date.now().toString(),
                        type: plantType,
                        name: plantName,
                        wateringOption: wateringOption,
                        growthStage: 'Sprout', // Default growth stage
                        image: null, // Default image
                    };
                    plants.push(newPlant);
                    localStorage.setItem('plants', JSON.stringify(plants));
                    renderPlantGrid();
                    addPlantModal.style.display = 'none';
                    addPlantForm.reset();
                });
        
                // Close modal when clicking outside
                window.addEventListener('click', (e) => {
                    if (e.target == addPlantModal) {
                        addPlantModal.style.display = 'none';
                    }
                });
            }   
        
                // ***** PLANT PAGE SCRIPT *****
    if (document.getElementById('plant-name-heading')) {
        // Variables
        let plants = JSON.parse(localStorage.getItem('plants')) || [];
        const currentPlantId = localStorage.getItem('currentPlantId');
        const plantIndex = plants.findIndex(p => p.id === currentPlantId);
        const plant = plants[plantIndex];

        if (!plant) {
            alert('Plant not found.');
            window.location.href = 'index.html';
        }

        const plantNameHeading = document.getElementById('plant-name-heading');
        const editPlantNameModal = document.getElementById('edit-plant-name-modal');
        const closeNameModalButton = document.getElementById('close-name-modal');
        const editPlantNameForm = document.getElementById('edit-plant-name-form');
        const plantImage = document.getElementById('plant-image');
        const editIcon = document.getElementById('edit-icon');
        const imageUpload = document.getElementById('image-upload');

        // Growth Stage Elements
        const growthStageSelect = document.getElementById('growth-stage-select');

        // New variables for settings menu
        const settingsIcon = document.getElementById('settings-icon');
        const settingsMenu = document.getElementById('settings-menu');
        const renamePlantOption = document.getElementById('rename-plant-option');
        const deletePlantOption = document.getElementById('delete-plant-option');

        // Delete Image Icon
        const deleteImageIcon = document.getElementById('delete-image-icon');

        // Set plant details
        plantNameHeading.textContent = plant.name;
        growthStageSelect.value = plant.growthStage || 'Sprout';

        // Function to update the plant image
        function updatePlantImage() {
            if (plant.image) {
                plantImage.src = plant.image;
            } else {
                const plantType = plant.type.toLowerCase().replace(/\s+/g, '_');
                const growthStage = plant.growthStage ? plant.growthStage.toLowerCase() : 'sprout';
                plantImage.src = `assets/grow_stages/${plantType}_${growthStage}.png`;
            }
        }

        updatePlantImage();

        // Settings Icon Functionality
        settingsIcon.addEventListener('click', (e) => {
            e.stopPropagation(); // Prevent event bubbling
            settingsMenu.style.display = settingsMenu.style.display === 'block' ? 'none' : 'block';
        });

        // Close the menu when clicking outside
        document.addEventListener('click', () => {
            settingsMenu.style.display = 'none';
        });

        // Rename Plant Option
        renamePlantOption.addEventListener('click', () => {
            settingsMenu.style.display = 'none';
            editPlantNameModal.style.display = 'block';
            document.getElementById('new-plant-name').value = plant.name;
        });

        // Delete Plant Option
        deletePlantOption.addEventListener('click', () => {
            settingsMenu.style.display = 'none';
            const confirmDelete = confirm('Are you sure you want to delete this plant?');
            if (confirmDelete) {
                plants.splice(plantIndex, 1);
                localStorage.setItem('plants', JSON.stringify(plants));
                window.location.href = 'index.html';
            }
        });

        // Close name modal
        closeNameModalButton.addEventListener('click', () => {
            editPlantNameModal.style.display = 'none';
        });

        // Save new plant name
        editPlantNameForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const newName = document.getElementById('new-plant-name').value;
            plant.name = newName;
            localStorage.setItem('plants', JSON.stringify(plants));
            plantNameHeading.textContent = newName;
            editPlantNameModal.style.display = 'none';
        });

        // Image upload
        editIcon.addEventListener('click', () => {
            imageUpload.click();
        });

        imageUpload.addEventListener('change', (event) => {
            const file = event.target.files[0];
            if (file && file.type.startsWith('image/')) {
                const reader = new FileReader();

                reader.onload = function (e) {
                    const imageBase64 = e.target.result;
                    resizeImage(imageBase64, 800, 600, (resizedImage) => {
                        plantImage.src = resizedImage;
                        plant.image = resizedImage;
                        localStorage.setItem('plants', JSON.stringify(plants));
                    });
                };

                reader.readAsDataURL(file);
            } else {
                alert('Please select a valid image file.');
            }
        });

        // Delete Image Functionality
        deleteImageIcon.addEventListener('click', () => {
            plant.image = null;
            localStorage.setItem('plants', JSON.stringify(plants));
            updatePlantImage();
        });


                // Function to fetch data from Firebase
        // Function to fetch data from Firebase
        function fetchPlantData() {
            const plantRef = ref(database, "/");

            onValue(plantRef, (snapshot) => {
                const data = snapshot.val();

                if (data) {
                    Object.keys(data).forEach((plantKey, index) => {
                        const plantData = data[plantKey];
                        const chartIdPrefix = `plant_${index + 1}`;

                        // Dynamically update graphs for each plant
                        updateGraph(`${chartIdPrefix}_moistureChart`, "Moisture (%)", plantData.moisture, 'rgba(54, 162, 235, 1)');
                        updateGraph(`${chartIdPrefix}_temperatureChart`, "Temperature (°C)", plantData.temperature, 'rgba(255, 99, 132, 1)');
                        updateGraph(`${chartIdPrefix}_conductivityChart`, "Electrical Conductivity (µS/cm)", plantData.electrical_conductivity, 'rgba(255, 206, 86, 1)');
                    });
                }
            });
        }


        // Function to update the graphs
        function updateGraph(chartId, label, value, color) {
            const ctx = document.getElementById(chartId).getContext('2d');
            const chart = Chart.getChart(ctx); // Check if chart already exists
        
            if (chart) {
                chart.data.datasets[0].data.push(value);
                chart.data.datasets[0].data.shift(); // Keep only 24 data points
                chart.update();
            } else {
                new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: Array(24).fill("").map((_, i) => `${24 - i}h`),
                        datasets: [{
                            label: label,
                            data: Array(24).fill(value),
                            backgroundColor: color.replace('1)', '0.2)'),
                            borderColor: color,
                            borderWidth: 2,
                            tension: 0.4,
                            fill: true
                        }]
                    },
                    options: {
                        responsive: true,
                        plugins: {
                            tooltip: { enabled: true }
                        },
                        scales: {
                            x: { title: { display: true, text: "Hours Ago" } },
                            y: { beginAtZero: true, title: { display: true, text: label } }
                        }
                    }
                });
            }
        }
        

        // Call fetchPlantData to load data
        fetchPlantData();

        // Resize image
        function resizeImage(imageBase64, maxWidth, maxHeight, callback) {
            const img = new Image();
            img.onload = function () {
                let width = img.width;
                let height = img.height;

                if (width > maxWidth) {
                    height *= maxWidth / width;
                    width = maxWidth;
                }
                if (height > maxHeight) {
                    width *= maxHeight / height;
                    height = maxHeight;
                }

                const canvas = document.createElement('canvas');
                canvas.width = width;
                canvas.height = height;
                const ctx = canvas.getContext('2d');
                ctx.drawImage(img, 0, 0, width, height);
                callback(canvas.toDataURL('image/jpeg', 0.8));
            };
            img.src = imageBase64;
        }

        // Handle Growth Stage Change
        growthStageSelect.addEventListener('change', () => {
            plant.growthStage = growthStageSelect.value;
            localStorage.setItem('plants', JSON.stringify(plants));
            if (!plant.image) {
                updatePlantImage();
            }
        });

        // Generate random data for the past 24 hours
        function generateRandomData() {
            return Array.from({ length: 24 }, () => Math.floor(Math.random() * 100));
        }

        // Generate labels for the past 24 hours
        function generateLabels() {
            return Array.from({ length: 24 }, (_, i) => `${23 - i}h`);
        }

        // Create charts
        function createChart(canvasId, label, data, color) {
            const ctx = document.getElementById(canvasId).getContext('2d');

            // Adjust the background color to have a 0.2 alpha value
            const backgroundColor = color.replace(/rgba\((\d+), (\d+), (\d+), [^)]+\)/, 'rgba($1, $2, $3, 0.2)');

            new Chart(ctx, {
                type: 'line',
                data: {
                    labels: generateLabels(),
                    datasets: [{
                        label: label,
                        data: data,
                        backgroundColor: backgroundColor, // Use the adjusted background color
                        borderColor: color,
                        borderWidth: 2,
                        pointBackgroundColor: color,
                        pointBorderColor: color,
                        pointRadius: 4,
                        pointHoverRadius: 6,
                        tension: 0.4,
                        fill: true,
                    }],
                },
                options: {
                    responsive: true,
                    interaction: {
                        mode: 'index',
                        intersect: false,
                    },
                    plugins: {
                        tooltip: {
                            enabled: true,
                        },
                    },
                    scales: {
                        x: {
                            title: {
                                display: true,
                                text: 'Hours Ago',
                            },
                        },
                        y: {
                            beginAtZero: true,
                            title: {
                                display: true,
                                text: label,
                            },
                        },
                    },
                },
            });
        }

        // Initialize charts with different colors
        createChart('moistureChart', 'Moisture (%)', generateRandomData(), 'rgba(54, 162, 235, 1)'); // Blue
        createChart('temperatureChart', 'Temperature (°C)', generateRandomData(), 'rgba(255, 99, 132, 1)'); // Red
        createChart('conductivityChart', 'Electrical Conductivity (µS/cm)', generateRandomData(), 'rgba(255, 206, 86, 1)'); // Yellow
    }
});