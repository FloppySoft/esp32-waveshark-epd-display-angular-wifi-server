import { Component, OnInit, ChangeDetectionStrategy, ViewChild, ElementRef } from '@angular/core';

export interface Vector2D { // TODO: Move reusable interface into root
  x: number;
  y: number;
}

@Component({
  selector: 'app-single-pic',
  templateUrl: './single-pic.component.html',
  styleUrls: ['./single-pic.component.scss'],
})
export class SinglePicComponent implements OnInit {
  @ViewChild('editCanvas', { static: true }) canvas: ElementRef<HTMLCanvasElement>;
  private ctx: CanvasRenderingContext2D;
  screenSize = { // TODO: Move into global config singleton
    x: 400,
    y: 300,
  };
  localUrl = null;
  isDragging = false;
  constructor() { }

  ngOnInit(): void {
  }

  onFileSelect(selectFileEvent: any) {
    if (selectFileEvent.target.files && selectFileEvent.target.files[0]) {
      this.previewImage(selectFileEvent.target.files[0]);
    }
  }

  onFileDrop(imageEvent: any) {
    const imageFile = imageEvent.dataTransfer.files[0];
    if (!imageFile.type.match(/image.*/)) {
      return;
    }
    this.previewImage(imageFile);
  }

  private previewImage(file: any): void {
    const reader = new FileReader();
    reader.onload = (event) => {
      this.localUrl = event.target.result;
      this.render(this.localUrl);
    },
      reader.readAsDataURL(file);
  }

  render(src) {
    const image = new Image();
    image.onload = () => {
      const size: Vector2D = { // Size of input image
        x: image.width,
        y: image.height,
      };
      const target: Vector2D = { // Target size (of this project's display)
        x: this.screenSize.x,
        y: this.screenSize.y,
      };
      const clipped: Vector2D = this.coverCrop(size, target); // Crop image to aspect ratio
      const offset = {  // Put image in middle
        x: (size.x - clipped.x) / 2,
        y: (size.y - clipped.y) / 2,
      };
      this.ctx = this.canvas.nativeElement.getContext('2d');
      this.canvas.nativeElement.width = target.x;
      this.canvas.nativeElement.height = target.y;
      this.ctx.drawImage(
        image,              // img    - Image to draw on canvas
        offset.x,           // sx     - Offset position, if aspect ratio not fitting
        offset.y,           // sy
        clipped.x,          // swidth - Width of cropped selection on original
        clipped.y,          // sheight
        0,                  // x      - Draw position
        0,                  // y
        target.x,  // width  - Drawing width on canvas
        target.y,  // heigt
      );
    };
    image.src = src;
  }

  private coverCrop(size: Vector2D, target: Vector2D): Vector2D {
    // Handle aspect ratio of input like a object-fit: cover
    const targetRatio = target.x / target.y;
    if (size.x / size.y > targetRatio) {
      // Wider than target image
      return ({
        x: size.y * (targetRatio),
        y: size.y,
      });
    } else {
      // Taller than target image (or fitting)
      return ({
        x: size.x,
        y: size.x / (targetRatio),
      });
    }
  }

}
