import { Component, OnInit, ChangeDetectionStrategy, ViewChild, ElementRef } from '@angular/core';
import { ImageService } from '../core/image.service';

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
  descriptionText = 'Add Picture';
  screenSize = { // TODO: Move into global config singleton
    x: 800,
    y: 480,
  };
  sourceUrl = null;
  previewUrl = null;
  byteArray: Uint8ClampedArray;
  isDragging = false;
  isPreview = false;
  adjustBrightness = 100;
  adjustContrast = 100;

  constructor(
    private imageService: ImageService,
    ) {}

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

  onTransmit(): void {
    this.imageService.putOnScreen(this.byteArray).subscribe(() => {

    });
  }

  private previewImage(file: any): void {
    const reader = new FileReader();
    reader.onload = (event) => {
      this.sourceUrl = event.target.result; // Set preview image
      this.render(this.sourceUrl);
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
      const ctx = this.canvas.nativeElement.getContext('2d') as CanvasRenderingContext2D;
      this.canvas.nativeElement.width = target.x;
      this.canvas.nativeElement.height = target.y;
      ctx.filter = `grayscale(100%) brightness(${this.adjustBrightness}%) contrast(${this.adjustContrast}%)`;
      ctx.drawImage(
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

      const imageData = ctx.getImageData(0, 0, target.x, target.y);
      const color = imageData.data;
      const bw = this.extractGreyscaleImageBytes(color);
      this.byteArray = this.dither(bw);
      const ditheredRgb = this.greyscaleToRbga(this.byteArray);
      const ditheredImage = new ImageData(ditheredRgb, this.screenSize.x, this.screenSize.y);
      ctx.putImageData(ditheredImage, 0, 0);
      this.previewUrl = this.canvas.nativeElement.toDataURL('image/jpeg');
    };
    image.src = src;
  }

  private extractGreyscaleImageBytes(rgbInput: Uint8ClampedArray): Uint8ClampedArray{
    const greyScaleImageBytes = rgbInput.filter((elem, i) => {
      return (i % 4) === 0;
    });
/*     for (let i = 0; i < this.screenSize.x * this.screenSize.y; i++) {
      greyScaleImageBytes.push(rgbInput[i * 4]);
    } */
    return greyScaleImageBytes;
  }

  private greyscaleToRbga(bwInput: Uint8ClampedArray): Uint8ClampedArray{
    const color: number[] = [];
    for (const elem of bwInput) {
      color.push(elem);
      color.push(elem);
      color.push(elem);
      color.push(255);
    }
    return new Uint8ClampedArray(color);
  }

  private toBlob(): void{
    /* const ctx = this.canvas.nativeElement.getContext('2D') as CanvasRenderingContext2D;
    const bytes = ctx.getImageData(0, 0, this.screenSize.x, this.screenSize.y).data; */
    const b64String = this.canvas.nativeElement.toDataURL('image/jpeg');
    const b64RawString = b64String.replace(/data:image\/jpeg;base64,/g, '');
  }

  private dataURItoBlob(dataURI) {
    const byteString = window.atob(dataURI);
    const arrayBuffer = new ArrayBuffer(byteString.length);
    const int8Array = new Uint8Array(arrayBuffer);
    for (let i = 0; i < byteString.length; i++) {
      int8Array[i] = byteString.charCodeAt(i);
    }
    const blob = new Blob([int8Array], { type: 'image/jpeg' });
    return blob;
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

  private dither(image: Uint8ClampedArray): Uint8ClampedArray {
    const width = this.screenSize.x;
    const height = this.screenSize.y;
    const dithered = image;
    for (let i = 0; i < height; i++) {
      for (let j = 0; j < width; j++) {
        const ci = i * width + j;
        const cc = dithered[ci];
        const rc = (cc < 128 ? 0 : 255);
        const err = cc - rc;
        dithered[ci] = rc;
        if (j + 1 < width) {

          dithered[ci + 1] += (err * 7) / 16;
        }
        if (i + 1 === height) {
          continue;
        }
        if (j > 0) {
          dithered[ci + width - 1] += (err * 3) / 16;
        }
        dithered[ci + width] += (err * 5) / 16;
        if (j + 1 < width) {
          dithered[ci + width + 1] += (err * 1) / 16;
        }
      }
    }
    return dithered;
  }

}
