import flet as ft
import random

# Data Model
class ParkingSlot:
    def __init__(self, slot_Id):
        self.slot_Id = slot_Id
        self.current_Status = "Free"  # States: Free, Occupied, Reserved

def main(page: ft.Page):
    page.title = "Smart Parking HMI - Mock Mode"
    page.vertical_alignment = ft.MainAxisAlignment.START
    page.horizontal_alignment = ft.CrossAxisAlignment.CENTER
    page.theme_mode = ft.ThemeMode.DARK

    # Initialize 20 mocked slots
    slots_Data = [ParkingSlot(f"Slot {i}") for i in range(1, 21)]
    ui_Slots = {}  # Map ID to Flet Container

    def update_Ui():
        # Calculate available slots
        free_Count = sum(1 for s in slots_Data if s.current_Status == "Free")
        status_Text.value = f"Available Slots: {free_Count} / {len(slots_Data)}"
        
        # Update colors based on state
        for s in slots_Data:
            container = ui_Slots[s.slot_Id]
            if s.current_Status == "Free":
                container.bgcolor = ft.colors.GREEN_700
                container.content.controls[1].value = "Free"
            elif s.current_Status == "Occupied":
                container.bgcolor = ft.colors.RED_700
                container.content.controls[1].value = "Occupied"
            elif s.current_Status == "Reserved":
                container.bgcolor = ft.colors.BLUE_700
                container.content.controls[1].value = "Reserved"
        page.update()

    def slot_Clicked(e):
        # Driver view simulation: Click to reserve a free slot
        clicked_Id = e.control.data
        selected_Slot = next((s for s in slots_Data if s.slot_Id == clicked_Id), None)
        
        if selected_Slot and selected_Slot.current_Status == "Free":
            selected_Slot.current_Status = "Reserved"
            update_Ui()
            page.snack_bar = ft.SnackBar(ft.Text(f"{clicked_Id} successfully reserved!"), open=True)
            page.update()
        elif selected_Slot and selected_Slot.current_Status == "Reserved":
             page.snack_bar = ft.SnackBar(ft.Text(f"{clicked_Id} is already reserved."), open=True)
             page.update()

    def simulate_Sensor_Event(e):
        # ESP32 Simulation: Randomly park or remove a car
        random_Slot = random.choice(slots_Data)
        
        # Hardware sensors shouldn't override software reservations in a basic setup
        if random_Slot.current_Status != "Reserved": 
            random_Slot.current_Status = "Occupied" if random_Slot.current_Status == "Free" else "Free"
        
        update_Ui()
        page.snack_bar = ft.SnackBar(ft.Text(f"Hardware Event: {random_Slot.slot_Id} sensor updated!"), open=True)
        page.update()

    # --- UI Components ---
    status_Text = ft.Text(size=28, weight=ft.FontWeight.BOLD)

    # The Parking Grid
    parking_Grid = ft.GridView(
        expand=1,
        runs_count=5, # 5 slots per row
        max_extent=120,
        child_aspect_ratio=1.0,
        spacing=10,
        run_spacing=10,
    )

    # Build the visual containers
    for s in slots_Data:
        container = ft.Container(
            data=s.slot_Id,
            content=ft.Column([
                ft.Text(s.slot_Id, size=18, weight="bold", color=ft.colors.WHITE),
                ft.Text("Free", color=ft.colors.WHITE)
            ], alignment=ft.MainAxisAlignment.CENTER, horizontal_alignment=ft.CrossAxisAlignment.CENTER),
            alignment=ft.alignment.center,
            border_radius=8,
            on_click=slot_Clicked # Bind the reservation click event
        )
        ui_Slots[s.slot_Id] = container
        parking_Grid.controls.append(container)

    # Add components to the page
    page.add(
        ft.Row([status_Text], alignment=ft.MainAxisAlignment.CENTER),
        ft.ElevatedButton("Mock ESP32 Sensor Event", on_click=simulate_Sensor_Event, icon=ft.icons.SENSORS),
        ft.Divider(height=20, color=ft.colors.TRANSPARENT),
        parking_Grid
    )
    
    # Initial draw
    update_Ui()

# Run the app
ft.app(target=main)